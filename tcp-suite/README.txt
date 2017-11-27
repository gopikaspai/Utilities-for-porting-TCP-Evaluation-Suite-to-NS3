Changelog:
1. Now allows multiple app types in socket based tracing. BulkSend and On-off are added. Other applications may be added with ease.
If you require us to add any other app to be socket-traced ( cwnd, rtt, seqno), please ping us.
2. Divided the Stats class into three subclasses for easier handling.
 - LinkStats for link utilisation, queue length
 - Socket stats for cwnd, rtt and seqno
 - Sink stats for throughput
 Note that each type of stats requires one to pass the appropriate node using the Install method.
 The main reason this has been done is because different stats are measured at different nodes. This division will help us when we are doing parking lot topology.
