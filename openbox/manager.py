#!/usr/bin/env python
#
# Copyright (c) 2015 Pavel Lazar pavel.lazar (at) gmail.com
#
# The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED.
#####################################################################


"""
An OBSI's Manager.
"""
import functools
import socket
import time
import sys
import psutil
import config
import messages
import rest_server
from manager_exceptions import EngineNotRunningError, ProcessingGraphNotSetError, UnknownRequestedParameter, \
    UnsupportedModuleDataEncoding
from tornado import httpclient, gen, options, locks
from tornado.escape import json_decode, json_encode, url_escape
from tornado.log import app_log
from tornado.ioloop import IOLoop, PeriodicCallback
from configuration_builder import ConfigurationBuilder
from message_handler import MessageHandler
from message_sender import MessageSender
from watchdog import ProcessWatchdog
from push_message_receiver import PushMessageReceiver, PushMessageHandler
from message_router import MessageRouter
from uuid import getnode


class ManagerState:
    EMPTY = 0
    INITIALIZING = 10
    INITIALIZED = 20


def _get_full_uri(base, endpoint):
    return '{base}{endpoint}'.format(base=base, endpoint=endpoint)


def _start_remote_rest_server(bin_path, port, debug):
    # use the current interpreter to run the remote servers
    # this may be an issue with virtualenv or anaconda
    cmd = [sys.executable, bin_path, '--port={port}'.format(port=port), '--debug={debug}'.format(debug=debug),
           '--logging=debug']
    return psutil.Popen(cmd)


class Manager(object):
    def __init__(self):
        self._runner_process = None
        self._control_process = None
        self._watchdog = ProcessWatchdog(config.Watchdog.CHECK_INTERVAL)
        self.push_messages_receiver = PushMessageReceiver()
        self.config_builder = ConfigurationBuilder(config.Engine.CONFIGURATION_BUILDER)
        self.message_handler = MessageHandler(self)
        self.message_sender = MessageSender()
        self.message_router = MessageRouter(self.message_sender, self.message_handler.default_message_handler)
        self.state = ManagerState.EMPTY
        self._http_client = httpclient.HTTPClient()
        self._alert_registered = False
        self.obsi_id = getnode()  # A unique identifier of this OBSI
        self._engine_running = False
        self._engine_running_lock = locks.Lock()
        self._processing_graph_set = False
        self._engine_config_builder = None
        self._keep_alive_periodic_callback = None
        self._avg_cpu = 0
        self._avg_duration = 0
        self._last_casteling = 0
        self._supported_elements_types = []
        self._alert_messages_handler = None
        self._castle_messages_handler = None
        self._log_messages_handler = None

    def start(self):
        app_log.info("Starting components")
        self.state = ManagerState.INITIALIZING
        self._start_runner()
        self._start_control()
        self._start_watchdog()
        self._start_push_messages_receiver()
        self._start_configuration_builder()
        self._start_message_router()
        self._start_local_rest_server()
        app_log.info("All components active")
        self.state = ManagerState.INITIALIZED
        self._start_sending_keep_alive()
        self._send_hello_message()
        self._start_io_loop()

    def _start_runner(self):
        app_log.info("Starting EE Runner on port {port}".format(port=config.Runner.Rest.PORT))
        self._runner_process = _start_remote_rest_server(config.Runner.Rest.BIN, config.Runner.Rest.PORT,
                                                         config.Runner.Rest.DEBUG)
        if self._runner_process.is_running() and self._rest_server_listening(config.Runner.Rest.BASE_URI):
            app_log.info("EERunner REST Server running")
        else:
            app_log.error("EERunner REST Server not running")
            self.exit(1)
        if self._is_engine_supported(_get_full_uri(config.Runner.Rest.BASE_URI, config.Runner.Rest.Endpoints.ENGINES)):
            app_log.info("{engine} supported by EE Runner.".format(engine=config.Engine.NAME))
        else:
            app_log.error("{engine} is not supported by EE Runner".format(engine=config.Engine.NAME))
            self.exit(1)
        if self._set_engine(_get_full_uri(config.Runner.Rest.BASE_URI, config.Runner.Rest.Endpoints.ENGINES)):
            app_log.info("{engine} set".format(engine=config.Engine.NAME))
        else:
            app_log.error("{engine} not set by EERunner".format(engine=config.Engine.NAME))
            self.exit(1)
        if self._start_engine():
            app_log.info("{engine} started".format(engine=config.Engine.NAME))
        else:
            app_log.error("{engine} failed to start".format(engine=config.Engine.NAME))
            self.exit(1)
        self._engine_running = True
        self._alert_registered = self._register_alert_uri()
        app_log.info("Alert Registration status: {status}".format(status=self._alert_registered))

    def exit(self, exit_code):
        if self._runner_process:
            while self._runner_process.is_running():
                self._runner_process.kill()
        if self._control_process:
            while self._control_process.is_running():
                self._control_process.kill()

        exit(exit_code)

    def _rest_server_listening(self, base_uri):
        for _ in xrange(config.Manager.CONNECTION_RETRIES):
            try:
                self._http_client.fetch(base_uri)
                return True
            except httpclient.HTTPError:
                return True
            except KeyboardInterrupt:
                raise
            except socket.error:
                time.sleep(config.Manager.INTERVAL_BETWEEN_CONNECTION_TRIES)

        return False

    def _is_engine_supported(self, uri):
        engine_name = config.Engine.NAME
        try:
            response = self._http_client.fetch(uri)
            return engine_name in json_decode(response.body)
        except httpclient.HTTPError as e:
            app_log.error(e.response)
            return False

    def _set_engine(self, uri):
        engine_name = config.Engine.NAME
        try:
            self._http_client.fetch(uri, method="POST", body=json_encode(engine_name))
            return True
        except httpclient.HTTPError as e:
            app_log.error(e.response)
            return False

    def _start_engine(self):
        params = dict(processing_graph=config.Engine.BASE_EMPTY_CONFIG,
                      control_socket_type=config.Engine.CONTROL_SOCKET_TYPE,
                      control_socket_endpoint=config.Engine.CONTROL_SOCKET_ENDPOINT,
                      nthreads=config.Engine.NTHREADS,
                      push_messages_type=config.Engine.PUSH_MESSAGES_SOCKET_TYPE,
                      push_messages_endpoint=config.Engine.PUSH_MESSAGES_SOCKET_ENDPOINT,
                      push_messages_channel=config.Engine.PUSH_MESSAGES_CHANNEL)
        uri = _get_full_uri(config.Runner.Rest.BASE_URI, config.Runner.Rest.Endpoints.START)
        try:
            self._http_client.fetch(uri, method="POST", body=json_encode(params))
            return True
        except httpclient.HTTPError as e:
            app_log.error(e.response)
            return False

    def _register_alert_uri(self):
        uri = _get_full_uri(config.Runner.Rest.BASE_URI, config.Runner.Rest.Endpoints.REGISTER_ALERT_URL)
        alert_uri = _get_full_uri(config.RestServer.BASE_URI, config.RestServer.Endpoints.RUNNER_ALERT)
        try:
            self._http_client.fetch(uri, method='POST', body=url_escape(alert_uri))
            return True
        except httpclient.HTTPError as e:
            app_log.error(e.response)
            return False

    def _start_control(self):
        app_log.info("Starting EE Control on port {port}".format(port=config.Control.Rest.PORT))
        self._control_process = _start_remote_rest_server(config.Control.Rest.BIN, config.Control.Rest.PORT,
                                                          config.Control.Rest.DEBUG)
        if self._control_process.is_running() and self._rest_server_listening(config.Control.Rest.BASE_URI):
            app_log.info("EEControl REST Server running")
        else:
            app_log.error("EEControl REST Server not running")
            exit(1)
        if self._is_engine_supported(
                _get_full_uri(config.Control.Rest.BASE_URI, config.Control.Rest.Endpoints.ENGINES)):
            app_log.info("{engine} supported by EE Control".format(engine=config.Engine.NAME))
        else:
            app_log.error("{engine} is not supported by EE Control".format(engine=config.Engine.NAME))
            exit(1)
        if self._set_engine(_get_full_uri(config.Control.Rest.BASE_URI, config.Control.Rest.Endpoints.ENGINES)):
            app_log.info("{engine} set by EE Control".format(engine=config.Engine.NAME))
        else:
            app_log.error("{engine} not set by EE Control".format(engine=config.Engine.NAME))
            exit(1)
        if self._connect_control():
            app_log.info("EE Control client connected to engine")
        else:
            app_log.error("EE Control client couldn't connect to engine")
            exit(1)

    def _connect_control(self):
        params = dict(address=config.Control.SOCKET_ADDRESS, type=config.Control.SOCKET_TYPE)
        uri = _get_full_uri(config.Control.Rest.BASE_URI, config.Control.Rest.Endpoints.CONNECT)
        try:
            self._http_client.fetch(uri, method="POST", body=json_encode(params))
            return True
        except httpclient.HTTPError as e:
            app_log.error(e.response)
            return False

    def _start_watchdog(self):
        app_log.info("Starting ProcessWatchdog")
        self._watchdog.register_process(self._runner_process, self._process_died)
        self._watchdog.register_process(self._control_process, self._process_died)
        self._watchdog.start()

    @gen.coroutine
    def _process_died(self, process):
        if process == self._runner_process:
            app_log.error("EE Runner REST server has died")
            with (yield self._engine_running_lock.acquire()):
                self._engine_running = False
                # TODO: add recovering logic or at least send error
        elif process == self._control_process:
            app_log.error("EE Control REST server has died")
            # TODO: Add real handling
        else:
            app_log.error("Unknown process dies")

    @gen.coroutine
    def _send_castle_request(self, message):
        interval = time.time() - self._last_casteling
        if interval > config.Casteling.CASTELING_INTERVAL:
            message = json_decode(message)
            msg = messages.Castle(origin_dpid=self.obsi_id, block=message["block"])
            received = yield self.message_sender.send_message_ignore_response(msg, url=None)
            if not received:
                app_log.error("Could not send Castle request to OBC")
            self._last_casteling = time.time()

    def _start_push_messages_receiver(self):
        app_log.info("Starting PushMessagesReceiver and registering Alert and Castle handling")
        url = None  # this will force the message sender to use the URL based on the message type
        send_alert_messages = functools.partial(self.message_sender.send_push_messages, messages.Alert, self.obsi_id,
                                                url)
        self._alert_messages_handler = PushMessageHandler(send_alert_messages,
                                                          config.PushMessages.Alert.BUFFER_SIZE,
                                                          config.PushMessages.Alert.BUFFER_TIMEOUT)

        self.push_messages_receiver.register_message_handler('ALERT', self._alert_messages_handler.add)
        self.push_messages_receiver.register_message_handler('CASTLE', self._send_castle_request)
        self.push_messages_receiver.connect(config.PushMessages.SOCKET_ADDRESS,
                                            config.PushMessages.SOCKET_FAMILY,
                                            config.PushMessages.RETRY_INTERVAL)

    def _start_configuration_builder(self):
        app_log.info("Starting EE Configuration Builder")
        try:
            uri = _get_full_uri(config.Control.Rest.BASE_URI, config.Control.Rest.Endpoints.SUPPORTED_ELEMENTS)
            response = self._http_client.fetch(uri)
            self._supported_elements_types = set(json_decode(response.body))
            supported_blocks = set(self.config_builder.supported_blocks())
            blocks_from_engine = set(self.config_builder.supported_blocks_from_supported_engine_elements_types(
                self._supported_elements_types))
            if supported_blocks != blocks_from_engine:
                app_log.warning("There is a mismatched between supported blocks by OBSI "
                                "and supported blocks by engine")

        except httpclient.HTTPError:
            app_log.error("Unable to connect to EE control in order to get a list of supported elements types")

    @gen.coroutine
    def _start_message_router(self):
        app_log.info("Starting MessageRouter")
        self._register_messages_handler()
        yield self.message_router.start()

    def _register_messages_handler(self):
        app_log.info("Registering handlers for messages")
        for message, handler in self.message_handler.registered_message_handlers.iteritems():
            self.message_router.register_message_handler(message, handler)

    def _start_local_rest_server(self):
        app_log.info("Starting local REST server on port {port}".format(port=config.RestServer.PORT))
        rest_server.start(self)

    def _start_sending_keep_alive(self):
        self._keep_alive_periodic_callback = PeriodicCallback(self._send_keep_alive, config.KeepAlive.INTERVAL)
        self._keep_alive_periodic_callback.start()

    @gen.coroutine
    def _send_keep_alive(self):
        received = yield self.message_sender.send_message_ignore_response(messages.KeepAlive(dpid=self.obsi_id))
        if not received:
            app_log.error('KeepAlive message received an error response from OBC')

    @gen.coroutine
    def _send_hello_message(self):
        app_log.info("Creating and sending Hello Message")
        while True:
            hello_message = messages.Hello(dpid=self.obsi_id, version=config.OPENBOX_VERSION,
                                           capabilities=self.get_capabilities())
            received = yield self.message_sender.send_message_ignore_response(hello_message)
            if received:
                break
            else:
                app_log.error("Hello message received an error response from OBC")
                yield gen.sleep(config.Manager.INTERVAL_BETWEEN_CONNECTION_TRIES)

    def get_capabilities(self):
        proto_messages = []
        if config.Engine.Capabilities.MODULE_INSTALLATION:
            proto_messages.append(messages.AddCustomModuleRequest.__name__)
        if config.Engine.Capabilities.MODULE_REMOVAL:
            proto_messages.append(messages.RemoveCustomModuleRequest.__name__)
        processing_blocks = self.config_builder.supported_blocks_from_supported_engine_elements_types(
            self._supported_elements_types)
        match_fields = self.config_builder.supported_match_fields()
        complex_match = self.config_builder.supported_complex_match()
        protocol_analyser_protocols = self.config_builder.supported_protocol_analyser_protocols()

        return dict(proto_messages=proto_messages, processing_blocks=processing_blocks,
                    match_fields=match_fields, complex_match=complex_match,
                    protocol_analyser_protocols=protocol_analyser_protocols)

    def _start_io_loop(self):
        app_log.info("Starting the IOLoop")
        IOLoop.current().start()

    @gen.coroutine
    def handle_runner_alert(self, errors):
        with (yield self._engine_running_lock.acquire()):
            self._engine_running = False
        app_log.error("Engine stopped working: {errors}".format(errors=errors))

    @gen.coroutine
    def get_engine_global_stats(self):
        client = httpclient.AsyncHTTPClient(request_timeout=config.Manager.REQUEST_TIMEOUT)
        memory_uri = _get_full_uri(config.Runner.Rest.BASE_URI, config.Runner.Rest.Endpoints.MEMORY)
        cpu_uri = _get_full_uri(config.Runner.Rest.BASE_URI, config.Runner.Rest.Endpoints.CPU)
        uptime_uri = _get_full_uri(config.Runner.Rest.BASE_URI, config.Runner.Rest.Endpoints.UPTIME)

        memory, cpu, uptime = yield [client.fetch(memory_uri), client.fetch(cpu_uri), client.fetch(uptime_uri)]
        memory, cpu, uptime = json_decode(memory.body), json_decode(cpu.body), json_decode(uptime.body)

        cpu_count = cpu['cpu_count']
        current_load = cpu['cpu_percent'] / 100.0 / cpu_count
        duration = cpu['measurement_time']
        self._avg_cpu = (current_load * duration + self._avg_cpu * self._avg_duration) / (duration + self._avg_duration)
        self._avg_duration += duration
        stats = dict(memory_rss=memory['rss'], memory_vms=memory['vms'], memory_percent=memory['percent'],
                     cpus=cpu_count, current_load=current_load, avg_load=self._avg_cpu,
                     avg_minutes=self._avg_duration / 60.0, uptime=uptime['uptime'])
        raise gen.Return(stats)

    @gen.coroutine
    def reset_engine_global_stats(self):
        self._avg_cpu = 0
        self._avg_duration = 0

    @gen.coroutine
    def read_block_value(self, block_name, handler_name):
        with (yield self._engine_running_lock.acquire()):
            if not self._engine_running:
                raise EngineNotRunningError()
        if not self._processing_graph_set or self._engine_config_builder is None:
            raise ProcessingGraphNotSetError()
        else:
            (engine_element_name,
             engine_handler_name,
             transform_function) = self._engine_config_builder.translate_block_read_handler(block_name,
                                                                                            handler_name)
            element = url_escape(engine_element_name)
            handler = url_escape(engine_handler_name)
            uri = _get_full_uri(config.Control.Rest.BASE_URI,
                                config.Control.Rest.Endpoints.HANDLER_PATTERN.format(element=element,
                                                                                     handler=handler))
            client = httpclient.AsyncHTTPClient(request_timeout=config.Manager.REQUEST_TIMEOUT)
            response = yield client.fetch(uri)
            raise gen.Return(transform_function(json_decode(response.body)))

    @gen.coroutine
    def write_block_value(self, block_name, handler_name, value):
        with (yield self._engine_running_lock):
            if not self._engine_running:
                raise EngineNotRunningError()
        if not self._processing_graph_set or self._engine_config_builder is None:
            raise ProcessingGraphNotSetError()
        else:
            (engine_element_name,
             engine_handler_name,
             transform_function) = self._engine_config_builder.translate_block_write_handler(block_name,
                                                                                             handler_name)
            uri = _get_full_uri(config.Control.Rest.BASE_URI,
                                config.Control.Rest.Endpoints.HANDLER_PATTERN.format(element=engine_element_name,
                                                                                     handler=engine_handler_name))
            body = json_encode(transform_function(value))
            client = httpclient.AsyncHTTPClient(request_timeout=config.Manager.REQUEST_TIMEOUT)
            yield client.fetch(uri, method='POST', body=body)
            raise gen.Return(True)

    @gen.coroutine
    def set_processing_graph(self, required_modules, blocks, connections):
        processing_graph = dict(requirements=required_modules, blocks=blocks, connections=connections)
        self._engine_config_builder = self.config_builder.engine_config_builder_from_dict(
            processing_graph,
            config.Engine.REQUIREMENTS)
        engine_config = self._engine_config_builder.to_engine_config()
        app_log.debug("Setting processing graph to:\n%s" % engine_config)
        client = httpclient.AsyncHTTPClient(request_timeout=config.Manager.REQUEST_TIMEOUT)
        uri = _get_full_uri(config.Control.Rest.BASE_URI, config.Control.Rest.Endpoints.CONFIG)
        yield client.fetch(uri, method='POST', body=json_encode(engine_config))
        # make sure we are stable in the new config
        # the config is in the same URI but with a GET method
        response = yield client.fetch(uri)
        new_config = json_decode(response.body)
        if engine_config in new_config:
            self._processing_graph_set = True
        else:
            app_log.error("Unable to set processing graph")

    @gen.coroutine
    def set_parameters(self, params):
        config.KeepAlive.INTERVAL = params.get('keepalive_interval', config.KeepAlive.INTERVAL)
        config.PushMessages.Alert.BUFFER_SIZE = params.get('alert_messages_buffer_size',
                                                           config.PushMessages.Alert.BUFFER_SIZE)
        config.PushMessages.Alert.BUFFER_TIMEOUT = params.get('alert_messages_buffer_timeout',
                                                              config.PushMessages.Alert.BUFFER_TIMEOUT * 1000.0) / 1000.0
        config.PushMessages.Log.BUFFER_SIZE = params.get('log_messages_buffer_size',
                                                         config.PushMessages.Log.BUFFER_SIZE)
        config.PushMessages.Log.BUFFER_TIMEOUT = params.get('log_messages_buffer_timeout',
                                                            config.PushMessages.Log.BUFFER_TIMEOUT * 1000.0) / 1000.0
        old_server, old_port = config.PushMessages.Log.SERVER_ADDRESS, config.PushMessages.Log.SERVER_PORT
        config.PushMessages.Log.SERVER_ADDRESS = params.get('log_server_address',
                                                            config.PushMessages.Log.SERVER_ADDRESS)
        config.PushMessages.Log.SERVER_PORT = params.get('log_server_port', config.PushMessages.Log.SERVER_PORT)
        new_server, new_port = config.PushMessages.Log.SERVER_ADDRESS, config.PushMessages.Log.SERVER_PORT
        config.PushMessages.Log._SERVER_CHANGED = new_server != old_server or new_port != old_port

        self._update_components()

    def _update_components(self):
        # update keepalive
        if self._keep_alive_periodic_callback:
            self._keep_alive_periodic_callback.stop()
            self._start_sending_keep_alive()

        # update alert push messages
        if self._alert_messages_handler:
            self._alert_messages_handler.buffer_size = config.PushMessages.Alert.BUFFER_SIZE
            self._alert_messages_handler.buffer_timeout = config.PushMessages.Alert.BUFFER_TIMEOUT

        # update castling push messages
        if self._castle_messages_handler:
            self._castle_messages_handler.buffer_size = config.PushMessages.Alert.BUFFER_SIZE
            self._castle_messages_handler.buffer_timeout = config.PushMessages.Alert.BUFFER_TIMEOUT

        # update log push messages
        if config.PushMessages.Log.SERVER_PORT and config.PushMessages.Log.SERVER_PORT:
            url = "http://{host}:{port}/message/Log".format(host=config.PushMessages.Log.SERVER_ADDRESS,
                                                            port=config.PushMessages.Log.SERVER_PORT)
            send_log_messages = functools.partial(self.message_sender.send_push_messages, messages.Log,
                                                  self.obsi_id, url)
            if config.PushMessages.Log._SERVER_CHANGED:
                # better close it and make it start over
                if self._log_messages_handler:
                    self._log_messages_handler.close()
                self.push_messages_receiver.unregister_message_handler('LOG')
                self._log_messages_handler = PushMessageHandler(send_log_messages,
                                                                config.PushMessages.Log.BUFFER_SIZE,
                                                                config.PushMessages.Log.BUFFER_TIMEOUT)
                self.push_messages_receiver.register_message_handler('LOG', self._log_messages_handler.add)
        if self._log_messages_handler:
            self._log_messages_handler.buffer_size = config.PushMessages.Log.BUFFER_SIZE
            self._log_messages_handler.buffer_timeout = config.PushMessages.Log.BUFFER_TIMEOUT

    def get_parameters(self, parameters):
        result = dict(keepalive_interval=int(config.KeepAlive.INTERVAL),
                      alert_messages_buffer_size=config.PushMessages.Alert.BUFFER_SIZE,
                      alert_messages_buffer_timeout=int(config.PushMessages.Alert.BUFFER_TIMEOUT * 1000),
                      log_messages_buffer_size=config.PushMessages.Log.BUFFER_SIZE,
                      log_messages_buffer_timeout=int(config.PushMessages.Log.BUFFER_TIMEOUT * 1000),
                      log_server_address=config.PushMessages.Log.SERVER_ADDRESS,
                      log_server_port=config.PushMessages.Log.SERVER_PORT)
        if not parameters:
            # an empty list means they want all of them
            return result
        else:
            try:
                partial = {}
                for parameter in parameters:
                    partial[parameter] = result[parameter]

                return partial
            except KeyError as e:
                raise UnknownRequestedParameter("Request unknown parameter {parm}".format(parm=e.message))

    @gen.coroutine
    def add_custom_module(self, name, content, content_type, encoding, translation):
        if content:
            if encoding.lower() != 'base64':
                raise UnsupportedModuleDataEncoding("Unknown encoding '{enc}' for module content".format(enc=encoding))
            yield self._install_package(name, content, encoding.lower())
            yield self._update_running_config_with_package(name)
            yield self._update_supported_elements()
            self.config_builder.add_custom_module(name, translation)

    @gen.coroutine
    def _install_package(self, name, content, encoding):
        package = dict(name=name, data=content, encoding=encoding)
        client = httpclient.AsyncHTTPClient(request_timeout=config.Manager.REQUEST_TIMEOUT)
        uri = _get_full_uri(config.Runner.Rest.BASE_URI, config.Runner.Rest.Endpoints.INSTALL)
        yield client.fetch(uri, method='POST', body=json_encode(package))

    @gen.coroutine
    def _update_running_config_with_package(self, name):
        client = httpclient.AsyncHTTPClient(request_timeout=config.Manager.REQUEST_TIMEOUT)
        uri = _get_full_uri(config.Control.Rest.BASE_URI, config.Control.Rest.Endpoints.LOADED_PACKAGES)
        yield client.fetch(uri, method='POST', body=json_encode(name))

    @gen.coroutine
    def _update_supported_elements(self):
        client = httpclient.AsyncHTTPClient(request_timeout=config.Manager.REQUEST_TIMEOUT)
        uri = _get_full_uri(config.Control.Rest.BASE_URI, config.Control.Rest.Endpoints.SUPPORTED_ELEMENTS)
        response = yield client.fetch(uri)
        self._supported_elements_types = set(json_decode(response.body))


def main():
    options.define('port', default=config.RestServer.PORT, type=int, help="The server's port. ")
    options.define('debug', default=config.RestServer.DEBUG, type=bool, help='Start the server with debug options.')
    options.parse_command_line()
    manager = Manager()
    manager.start()


if __name__ == '__main__':
    main()