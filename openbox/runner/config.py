#!/usr/bin/env python
#
# Copyright (c) 2015 Pavel Lazar pavel.lazar (at) gmail.com
#
# The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED.
#####################################################################

"""
A configuration and definitions file used by the EE runner server and client
"""
from click_runner_client import ClickRunnerClient, NetworkStack


ENGINES = {'click': (ClickRunnerClient, dict(click_bin=r'/usr/local/bin/click', allow_reconfigure=True,
                                             click_path=r'/usr/local/lib'))}


class RestServer:
    ENGINE_START_PARAMETERS = ('processing_graph', 'control_socket_type', 'control_socket_endpoint', 'nthreads',
                               'push_messages_type', 'push_messages_endpoint', 'push_messages_channel', 'network_stack')
    PORT = 9001
    DEBUG = True
    CLIENT_RUN_POLLING_INTERVAL = 500  # Milliseconds

    class Endpoints:
        ENGINES = '/runner/engines'
        START = '/runner/start'
        SUSPEND = '/runner/suspend'
        RESUME = '/runner/resume'
        STOP = '/runner/stop'
        RUNNING = '/runner/running'
        MEMORY = '/runner/memory'
        CPU = '/runner/cpu'
        UPTIME = '/runner/uptime'
        INSTALL = '/runner/install_package'
        REGISTER_ALERT_URL = '/runner/register_alert_url'

