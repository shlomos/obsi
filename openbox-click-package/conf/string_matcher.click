
require(package "openbox");
FromDump(/home/mininet/hello_rest.pcap, STOP true) -> 
sm::StringMatcher(MATCHER "ahocorasick", "discover");
sm[0] -> ToDump(output_not_discover.pcap, SNAPLEN 0, ENCAP ETHER);
sm[1] -> ToDump(output_discover.pcap, SNAPLEN 0, ENCAP ETHER);

