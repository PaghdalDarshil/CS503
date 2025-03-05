1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

_Ans: - The client identifies completion by examining the EOF marker (RDSH_EOF_CHAR = 0x04) supplied by the server. Because the TCP does not preserve message boundaries, the client must call recv() repeatedly until it receives this marker. This techniques include buffering received data, verifying if the last byte is the EOF character, and replacing it with '\0' for safe string processing. Error handling detects disconnections (recv() returns 0) and failures (recv() returns -1). Proper handling prevents command outputs that are incomplete or combined._

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

_Ans: - Client commands should be null-terminated ('\0'), whereas the server responses should finish with the EOF (0x04). This makes sure that the message integrity despite the fact that the TCP is a stream-based protocol. Without clear delimiters, messages can divide unexpectedly, resulting in partial execution or combined outputs. If the client waits forever for the missing delimiters, the shell could fail. If data is misunderstood, incomplete commands may run incorrectly. Proper boundary management prevents these problems and promotes effective communication._

3. Describe the general differences between stateful and stateless protocols.

_Ans: - Stateful protocols save the session data in requests, making it possible to track the interactions. whereas the TCP, for example, ensures that data is delivered in order and securely. Stateless protocols handle each request independently, without saving past state. Consider the UDP, which transfers the packets without confirmation. Stateful protocols can be useful for SSH, FTP, and streaming, whereas stateless protocols are recommended for DNS, HTTP, and IoT, which require less overhead._

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

_Ans: - UDP is faster than the TCP because it does not require connection setup, retransmission, or error checking. It is useful for real-time applications such as VoIP, gaming, video streaming, and IoT devices, where minimal data loss is manageable but delays are not. It also supports multicasting/broadcasting, unlike the TCP. The DNS employs UDP to provide quick query results as retries are faster than maintaining a connection. Some of the apps achieve reliability on the application level while taking use of the UDP's reduced latency._

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

_Ans: - The Berkeley Sockets API (BSD Sockets) allows the applications to communicate over the network. It includes the routines for creating sockets (socket()), binding the addresses (bind()), establishing the connections (connect()), listening for the clients (listen()), sending and receiving the data (send(), recv()), and closing the connections (close()). It can handle TCP, UDP, and raw sockets.Here the windows utilizes Winsock, while the POSIX systems use POSIX Sockets. WebSockets allow the web apps to communicate in the real time._
