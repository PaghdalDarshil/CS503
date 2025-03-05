1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

Ans: - By using waitpid() on each child process, our shell guarantees that they all complete before continuing. This is done in a loop after forking processes in execute_pipeline(). Each waitpid() call suspends execution until the associated child process quits, ensuring that all forked processes are cleaned up sequentially before the shell allows fresh user input. What Happens If We Forget Waitpid()? Zombie Processes: - If waitpid() is not invoked, terminated child processes persist in the process table as "zombies," eating system resources. Uncontrolled Execution Flow: - The shell may continue reading user input before all child processes have completed, resulting in inconsistent results. Process ID Reuse Issues: - If there are too many zombies, the system may run out of available PIDs, leading fresh forks to fail.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?
   
Ans: - The dup2() function is the replacement of the conventional input and output file descriptors with the appropriate pipe file descriptors. However, unused pipe ends must be closed to avoid resource leaks and unpredictable behavior. What Could Go Wrong If Pipes Stay Open? Hanging Processes (Deadlocks): - If a process leaves a pipe open, it may wait endlessly for input that never arrives. File Descriptor Exhaustion: - The system has a restricted number of file descriptors. Leaving unused pipes open uses file descriptors and prevents other operations from executing. Data Corruption: - Having the unused write ends open can result in incomplete readings or erroneous ordering of piped data.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?
   
Ans: - The cd command is implemented as a built-in function rather than an external command since it requires changing the shell's current working directory.
Why Can't cd be an External Command? 
Each process has its own working directory; when fork() is used, a child process inherits a copy of its parent's environment. If the cd were an external command, the child process would change the directory. 
However, the parent shell's directory will remain unchanged once the child quits.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?
   
Ans: - Currently, the shell limits piped commands to a fixed array (CMD_MAX). In order to allow for limitless piped instructions, a dynamic allocation of memory is required. Instead of a static array, commands and pipes should be allocated dynamically in response to user input.
Trade-offs: -
Higher complexity: - Requires careful memory management to prevent leaks. 
Performance Overhead: - Frequent allocations can slow down execution. 
Memory Fragmentation: - Continuous allocation/deallocation might deplete memory. 
Despite these trade-offs, dynamic allocation allows the shell to be scalable by optimizing memory utilization while efficiently supporting any number of pipes.
