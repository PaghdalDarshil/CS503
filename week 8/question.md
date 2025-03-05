1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

_Ans: - The client identifies completion by examining the EOF marker (RDSH_EOF_CHAR = 0x04) supplied by the server.  Because TCP does not preserve message boundaries, the client must call recv() repeatedly until it receives this marker.  Techniques include buffering received data, verifying if the last byte is the EOF character, and replacing it with '\0' for safe string processing.  Error handling detects disconnections (recv() returns 0) and failures (recv() returns -1).  Proper handling prevents command outputs that are incomplete or concatenated._

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

_answer here_

3. Describe the general differences between stateful and stateless protocols.

_answer here_

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

_answer here_

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

_answer here_
