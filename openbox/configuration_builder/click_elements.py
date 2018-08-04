#!/usr/bin/env python
#
# Copyright (c) 2015 Pavel Lazar pavel.lazar (at) gmail.com
#
# The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED.
#####################################################################
import json

from configuration_builder_exceptions import ClickElementConfigurationError


class Argument(object):
    """
    Base class for all argument of an element's configuration
    """

    def __init__(self, name):
        self.name = name

    def from_dict(self, config, default=None):
        """
        Parse the variable from the configuration of the element

        :param config: The element's config.
        :type config: dict
        :param default: The default value to use when there is no argument
        """
        return config.get(self.name, default)

    def to_click_argument(self, value):
        """
        Get click's argument representation
        """
        if isinstance(value, bool):
            return str(value).lower()
        return str(value)

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

    def __ne__(self, other):
        return not self.__eq__(other)


class MandatoryPositionalArgument(Argument):
    """
    A mandatory positional argument
    """

    def from_dict(self, config, default=None):
        value = super(MandatoryPositionalArgument, self).from_dict(config, default)
        if value is None:
            raise ClickElementConfigurationError("No mandatory argument named: {name}".format(name=self.name))
        else:
            return value


class OptionalPositionalArgument(Argument):
    """
    An optional positional argument
    """
    pass


class KeywordArgument(Argument):
    """
    A keyword argument
    """

    def to_click_argument(self, value):
        if value is not None:
            value = super(KeywordArgument, self).to_click_argument(value)
            return '{name} {value}'.format(name=self.name.upper(), value=value)
        else:
            return None


class ListArguments(Argument):
    """
    A list of arguments with the same name.
    """

    def from_dict(self, config, default=None):
        value = config.get(self.name, default or [])
        return value

    def to_click_argument(self, value):
        if not isinstance(value, (list, tuple)):
            raise TypeError("Value should be a list/tuple and not %s" % type(value))
        return ', '.join(super(ListArguments, self).to_click_argument(val) for val in value)


class ElementMeta(type):
    def __init__(cls, name, bases, dct):
        if not hasattr(cls, "elements_registry"):
            # this is the base class. Create an empty registry
            cls.elements_registry = {}
        else:
            # this is the derived class. Add cls to the registry
            cls.elements_registry[name] = cls

        super(ElementMeta, cls).__init__(name, bases, dct)


class Element(object):
    """
    The base class for all elements
    """
    __metaclass__ = ElementMeta
    __list_arguments__ = None
    __mandatory_positional_arguments__ = ()
    __optional_positional_arguments__ = ()
    __keyword_arguments__ = ()
    __ELEMENT_PATTERN = "{name}::{type}({args});"
    __read_handlers__ = []
    __write_handlers__ = []

    def __init__(self, name, **kwargs):
        self.name = name
        for arg_name, arg_value in kwargs.iteritems():
            setattr(self, arg_name, arg_value)

    @classmethod
    def from_dict(cls, config):
        """
        Create an instance of an element from the elements configuration dict

        :param config: The elements configuration
        :type config: dict
        :return: An instance of a specific type
        :rtype: Element
        """
        element_type = config.get('type')
        if element_type is None:
            raise ClickElementConfigurationError("No element type is given in the element's configuration")
        # noinspection PyUnresolvedReferences
        clazz = cls.elements_registry.get(element_type)
        if clazz is None:
            raise ClickElementConfigurationError("Unknown element type %s" % element_type)
        name = config.get('name')
        if name is None:
            raise ClickElementConfigurationError("An element must have an instance name")
        element = clazz(name)
        config = config.get('config', {})
        if clazz.__list_arguments__ is not None:
            value = clazz.__list_arguments__.from_dict(config)
            setattr(element, clazz.__list_arguments__.name, value)

        for arg in clazz.__mandatory_positional_arguments__:
            value = arg.from_dict(config)
            setattr(element, arg.name, value)
        for arg in clazz.__optional_positional_arguments__:
            value = arg.from_dict(config)
            setattr(element, arg.name, value)
        for arg in clazz.__keyword_arguments__:
            value = arg.from_dict(config)
            setattr(element, arg.name, value)
        return element

    def to_click_config(self):
        """
        Create Click's configuration string of the element

        :rtype: str
        """
        config_args = []
        # Variable length list of arguments is mutually exclusive with positional arguments
        if self.__list_arguments__:
            arg_value = getattr(self, self.__list_arguments__.name)
            config_args.append(self.__list_arguments__.to_click_argument(arg_value))
        else:
            for arg in self.__mandatory_positional_arguments__:
                arg_value = getattr(self, arg.name)
                config_args.append(arg.to_click_argument(arg_value))
            for arg in self.__optional_positional_arguments__:
                arg_value = getattr(self, arg.name, None)
                if arg_value is not None:
                    config_args.append(arg.to_click_argument(arg_value))

        # keywords argument can always be present
        for arg in self.__keyword_arguments__:
            arg_value = getattr(self, arg.name, None)
            if arg_value is not None:
                config_args.append(arg.to_click_argument(arg_value))

        args = ', '.join(config_args)
        return self.__ELEMENT_PATTERN.format(name=self.name, type=self.__class__.__name__, args=args)

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False

        args = set(self.__dict__.keys())
        args.update(other.__dict__.keys())
        for arg in args:
            this_arg = getattr(self, arg, None)
            other_arg = getattr(other, arg, None)
            if this_arg != other_arg:
                return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)

    def __str__(self):
        return self.to_click_config()


def build_element(name, list_argument=None, mandatory_positional=None, optional_positional=None, keywords=None,
                  read_handlers=None, write_handlers=None):
    """
    Create an element class based on the arguments it receives.

    :param name: Element's name.
    :type name: str
    :param list_argument: The base name for a variable amount of arguments of the same type.
    :type list_argument: ListArguments
    :param mandatory_positional: A list of mandatory positional arguments.
    :type mandatory_positional: list(MandatoryPositionalArgument) | tuple(MandatoryPositionalArgument)
    :param optional_positional: A list of optional positional arguments.
    :type optional_positional: list(OptionalPositionalArgument) | tuple(OptionalPositionalArgument)
    :param keywords: A list of keywords arguments
    :type keywords: list(KeywordArgument) | tuple(KeywordArgument)
    :param read_handlers: Names of the element's read handlers
    :type read_handlers: list(str) | None
    :param write_handlers: Names of the element's read handlers
    :type write_handlers: list(str) | None
    :rtype: Element
    """
    if list_argument is not None and (mandatory_positional is not None or optional_positional is not None):
        raise ValueError("An element cannot have a list of arguments and positional arguments")
    if list_argument is not None and not isinstance(list_argument, ListArguments):
        raise TypeError("list_argument must be a ListArguments not %s" % type(list_argument))
    if mandatory_positional is not None:
        if not (isinstance(mandatory_positional, (list, tuple)) and all(
                isinstance(arg, MandatoryPositionalArgument) for arg in mandatory_positional)):
            raise TypeError("mandatory_positional must be a list/tuple of MandatoryPositionalArgument")
    else:
        mandatory_positional = ()
    if optional_positional is not None:
        if not (isinstance(optional_positional, (list, tuple)) and all(
                isinstance(arg, OptionalPositionalArgument) for arg in optional_positional)):
            raise TypeError("optional_positional must be a list/tuple of OptionalPositionalArgument")
    else:
        optional_positional = ()

    keywords = keywords or ()
    read_handlers = read_handlers or ()
    write_handlers = write_handlers or ()
    element_arguments = {'__list_arguments__': list_argument,
                         '__mandatory_positional_arguments__': tuple(mandatory_positional),
                         '__optional_positional_arguments__': tuple(optional_positional),
                         '__keyword_arguments__': tuple(keywords),
                         '__read_handlers__': tuple(read_handlers),
                         '__write_handlers__': tuple(write_handlers),
                         }

    return ElementMeta(name, (Element,), element_arguments)


def build_element_from_dict(element):
    if not isinstance(element, dict):
        raise TypeError("Cannot build element from '%s'" % type(element))
    name = element['name']
    list_argument = None
    mandatory_positional = None
    optional_positional = None
    keywords = None
    read_handlers = None
    write_handlers = None
    if 'list_argument' in element:
        list_argument = ListArguments(element['list_argument'])
    if 'mandatory_positional' in element and isinstance(element['mandatory_positional'], (tuple, list)):
        mandatory_positional = [MandatoryPositionalArgument(arg) for arg in element['mandatory_positional']]
    if 'optional_positional' in element and isinstance(element['optional_positional'], (tuple, list)):
        optional_positional = [OptionalPositionalArgument(arg) for arg in element['optional_positional']]
    if 'keywords' in element and isinstance(element['keywords'], (tuple, list)):
        keywords = [KeywordArgument(arg) for arg in element['keywords']]
    if 'read_handlers' in element and isinstance(element['read_handlers'], (tuple, list)):
        read_handlers = [handler for handler in element['read_handlers']]
    if 'write_handlers' in element and isinstance(element['write_handlers'], (tuple, list)):
        write_handlers = [handler for handler in element['write_handlers']]
    return build_element(name, list_argument, mandatory_positional, optional_positional, keywords, read_handlers,
                         write_handlers)


def build_element_from_json(element):
    return build_element_from_dict(json.loads(element))


Idle = build_element('Idle')
DiscardNoFree = build_element('DiscardNoFree')
InfiniteSource = build_element('InfiniteSource',
                               optional_positional=(OptionalPositionalArgument('data'),
                                                    OptionalPositionalArgument('limit'),
                                                    OptionalPositionalArgument('burst'),
                                                    OptionalPositionalArgument('active')),
                               keywords=(KeywordArgument('length'),
                                         KeywordArgument('stop'),
                                         KeywordArgument('end_call'),
                                         KeywordArgument('timestamp')))

RandomSource = build_element('RandomSource',
                             mandatory_positional=[MandatoryPositionalArgument('length')],
                             optional_positional=(OptionalPositionalArgument('limit'),
                                                  OptionalPositionalArgument('burst'),
                                                  OptionalPositionalArgument('active')),
                             keywords=(KeywordArgument('stop'),
                                       KeywordArgument('end_call')))
SimpleIdle = build_element('SimpleIdle')
TimedSink = build_element('TimedSink', optional_positional=[OptionalPositionalArgument('interval')])

CheckAverageLength = build_element('CheckAverageLength',
                                   mandatory_positional=[MandatoryPositionalArgument('minlength')])

Classifier = build_element('Classifier', list_argument=ListArguments('pattern'))
FromDevice = build_element('FromDevice', mandatory_positional=[MandatoryPositionalArgument('devname')],
                           keywords=[KeywordArgument('sniffer'),
                                     KeywordArgument('promisc'),
                                     KeywordArgument('snaplen'),
                                     KeywordArgument('force_ip'),
                                     KeywordArgument('method'),
                                     KeywordArgument('bpf_filter'),
                                     KeywordArgument('encap'),
                                     KeywordArgument('outbound'),
                                     KeywordArgument('headroom'),
                                     KeywordArgument('burst'),
                                     ],
                           read_handlers=['count', 'kernel-drops', 'encap'],
                           write_handlers=['reset_counts'])
Counter = build_element('Counter',
                        read_handlers=['count', 'byte_count', 'rate', 'byte_rate'],
                        write_handlers=['reset_counts'])

FromDump = build_element('FromDump', mandatory_positional=[MandatoryPositionalArgument('filename')],
                         keywords=[KeywordArgument('stop'),
                                   KeywordArgument('timing'),
                                   KeywordArgument('sample'),
                                   KeywordArgument('force_ip'),
                                   KeywordArgument('start'),
                                   KeywordArgument('start_after'),
                                   KeywordArgument('end'),
                                   KeywordArgument('end_after'),
                                   KeywordArgument('interval'),
                                   KeywordArgument('end_call'),
                                   KeywordArgument('active'),
                                   KeywordArgument('filepos'),
                                   KeywordArgument('mmap'),
                                   ],
                         read_handlers=['count', 'active'],
                         write_handlers=['reset_counts', 'reset_timing'])

Discard = build_element('Discard',
                        keywords=[KeywordArgument('active'),
                                  KeywordArgument('burst'),
                                  ],
                        read_handlers=['count', 'active'],
                        write_handlers=['reset_counts'])

AutoMarkIPHeader = build_element('AutoMarkIPHeader')

ToDevice = build_element('ToDevice',
                         mandatory_positional=[
                             MandatoryPositionalArgument('devname')
                         ],
                         optional_positional=[
                             OptionalPositionalArgument('burst')
                         ],
                         keywords=[
                             KeywordArgument('quiet'),
                             KeywordArgument('queue'),
                             KeywordArgument('allow_nonexistent'),
                             KeywordArgument('no_pad'),
                         ],
                         read_handlers=[

                         ]
                         )
ToDump = build_element('ToDump',
                       mandatory_positional=[
                           MandatoryPositionalArgument('filename')
                       ],
                       keywords=[
                           KeywordArgument('snaplen'),
                           KeywordArgument('encap'),
                           KeywordArgument('use_encap_from'),
                           KeywordArgument('extra_length'),
                           KeywordArgument('unbuffered'),
                       ],
                       read_handlers=['count', 'calls', 'drops', ],
                       write_handlers=['reset_counts']
                       )

PushMessage = build_element('PushMessage',
                            mandatory_positional=[
                                MandatoryPositionalArgument('type'),
                                MandatoryPositionalArgument('msg'),
                            ],
                            keywords=[
                                KeywordArgument('active'),
                                KeywordArgument('channel'),
                                KeywordArgument('attach_packet'),
                                KeywordArgument('packet_size'),
                            ],
                            read_handlers=[],
                            write_handlers=[])

MultiCounter = build_element('MultiCounter',
                             read_handlers=['count', 'byte_count', 'rate', 'byte_rate'],
                             write_handlers=['reset_counts'])

IPClassifier = build_element('IPClassifier',
                             list_argument=ListArguments('pattern'),
                             read_handlers=['program', 'pattern$i'],
                             write_handlers=['pattern$i']
                             )

RegexMatcher = build_element("RegexMatcher",
                             list_argument=ListArguments('pattern'),
                             keywords=[
                                 KeywordArgument('payload_only'),
                                 KeywordArgument('match_all'),
                             ],
                             read_handlers=['payload_only', 'match_all', 'pattern$i'],
                             write_handlers=['payload_only', 'match_all', 'pattern$i'])

RegexClassifier = build_element("RegexClassifier",
                                list_argument=ListArguments('pattern'),
                                keywords=[KeywordArgument('payload_only'),
                                          KeywordArgument('max_regex_memory')],
                                read_handlers=['payload_only', 'pattern$i'],
                                write_handlers=['payload_only', 'pattern$i'])

GroupRegexClassifier = build_element("GroupRegexClassifier",
                                     list_argument=ListArguments('pattern'),
                                     read_handlers=['pattern$i'],
                                     write_handlers=['pattern$i'])

VLANDecap = build_element('VLANDecap',
                          keywords=[KeywordArgument('anno')])

VLANEncap = build_element('VLANEncap',
                          mandatory_positional=[
                              MandatoryPositionalArgument('vlan_tci'),
                              MandatoryPositionalArgument('vlan_pcp'),
                          ],
                          keywords=[
                              KeywordArgument('vlan_id'),
                              KeywordArgument('native_vlan'),
                              KeywordArgument('ethertype'),
                          ],
                          read_handlers=['vlan_tci', 'vlan_pcp', 'vlan_id', 'native_vlan', 'ethertype'],
                          write_handlers=['vlan_tci', 'vlan_pcp', 'vlan_id', 'native_vlan', 'ethertype'],
                          )

DecIPTTL = build_element('DecIPTTL',
                         keywords=[KeywordArgument('active'), KeywordArgument('multicast')],
                         read_handlers=['active', 'multicast'],
                         write_handlers=['active', 'multicast'])

IPRewriter = build_element('IPRewriter',
                           list_argument=ListArguments('input_spec'),
                           keywords=[
                               KeywordArgument('tcp_timeout'),
                               KeywordArgument('tcp_done_timeout'),
                               KeywordArgument('tcp_nodata_timeout'),
                               KeywordArgument('tcp_guarantee'),
                               KeywordArgument('udp_timeout'),
                               KeywordArgument('udp_streaming_timeout'),
                               KeywordArgument('udp_guarantee'),
                               KeywordArgument('reap_interval'),
                               KeywordArgument('mapping_capacity'),
                               KeywordArgument('dst_anno'),
                           ],
                           read_handlers=['nmappings', 'mapping_failures', 'length', 'capacity', 'tcp_mappings',
                                          'udp_mappings'],
                           write_handlers=['capacity']
                           )

Queue = build_element('Queue',
                      optional_positional=[OptionalPositionalArgument('capacity')],
                      read_handlers=['length', 'highwater_length', 'capacity', 'drops'],
                      write_handlers=['reset_counts', 'reset']
                      )

NetworkDirectionSwap = build_element('NetworkDirectionSwap',
                                     keywords=[
                                         KeywordArgument('ethernet'),
                                         KeywordArgument('ipv4'),
                                         KeywordArgument('ipv6'),
                                         KeywordArgument('tcp'),
                                         KeywordArgument('udp'),
                                     ],
                                     read_handlers=['ethernet', 'ipv4', 'ipv6', 'tcp', 'udp'],
                                     write_handlers=['ethernet', 'ipv4', 'ipv6', 'tcp', 'udp'])

NetworkHeaderFieldsRewriter = build_element('NetworkHeaderFieldsRewriter',
                                            keywords=[
                                                KeywordArgument('eth_src'),
                                                KeywordArgument('eth_dst'),
                                                KeywordArgument('eth_type'),
                                                KeywordArgument('ipv4_proto'),
                                                KeywordArgument('ipv4_src'),
                                                KeywordArgument('ipv4_dst'),
                                                KeywordArgument('ipv4_dscp'),
                                                KeywordArgument('ipv4_ecn'),
                                                KeywordArgument('ipv4_ttl'),
                                                KeywordArgument('tcp_src'),
                                                KeywordArgument('tcp_dst'),
                                                KeywordArgument('udp_src'),
                                                KeywordArgument('udp_dst'),
                                            ],
                                            read_handlers=['eth_src', 'eth_dst', 'eth_type',
                                                           'ipv4_proto', 'ipv4_src', 'ipv4_dst', 'ipv4_ecn', 'ipv4dsvp',
                                                           'ipv4_ttl', 'tcp_src', 'tcp_dst', 'udp_src', 'udp_dst'],
                                            write_handlers=['eth_src', 'eth_dst', 'eth_type',
                                                            'ipv4_proto', 'ipv4_src', 'ipv4_dst', 'ipv4_ecn',
                                                            'ipv4dsvp', 'ipv4_ttl', 'tcp_src', 'tcp_dst', 'udp_src',
                                                            'udp_dst'])

Unqueue = build_element('Unqueue',
                        keywords=[
                            KeywordArgument('active'),
                            KeywordArgument('limit'),
                            KeywordArgument('burst'),
                        ],
                        read_handlers=['count', 'active', 'limit', 'burst'],
                        write_handlers=['reset', 'count', 'limit', 'burst'])

SetTimestamp = build_element('SetTimestamp',
                             optional_positional=[
                                 OptionalPositionalArgument('timestamp'),
                             ],
                             keywords=[
                                 KeywordArgument('first')
                             ])

SetTimestampDelta = build_element('SetTimestampDelta',
                                  keywords=[
                                      KeywordArgument('type')
                                  ],
                                  read_handlers=['first'],
                                  write_handlers=['reset'])

StringClassifier = build_element("StringClassifier",
                                 keywords=[KeywordArgument('matcher')],
                                 list_argument=ListArguments('pattern'),
                                 read_handlers=['pattern$i'],
                                 write_handlers=['pattern$i'])

UtilizationMonitor = build_element('UtilizationMonitor',
        keywords=[
            KeywordArgument('window'),
            KeywordArgument('proc_threshold')
        ],
        read_handlers=['count', 'time', 'average_time', 'average_window_time'],
        write_handlers=['reset'])

StringMatcher = build_element("StringMatcher",
                                 keywords=[KeywordArgument('matcher')],
                                 list_argument=ListArguments('pattern'),
                                 read_handlers=['pattern$i'],
                                 write_handlers=['pattern$i'])
