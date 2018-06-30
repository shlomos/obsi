require(package "openbox")
ChatterSocket("TCP", 10002, RETRIES 3, RETRY_WARNINGS false, CHANNEL openbox);
ControlSocket("TCP", 10001, RETRIES 3, RETRY_WARNINGS false);
alert::ChatterMessage("ALERT", "{\"message\": \"This is a test alert\", \"origin_block\": \"alert\", \"packet\": \"00 00 00 00\"}", CHANNEL openbox);
log::ChatterMessage("LOG", "{\"message\": \"This is a test log\", \"origin_block\": \"log\", \"packet\": \"00 00 00 00\"}", CHANNEL openbox);
timed_source1::TimedSource(10, "base");
timed_source2::TimedSource(5, "base");
um::UtilizationMonitor(10, 2);
discard::Discard();
timed_source1 -> SetTimestamp -> [0]um;
timed_source2 -> SetTimestamp -> [1]um;
um[0] -> log -> discard;
um[1] -> alert -> discard;