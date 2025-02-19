1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**: The fork system call produce a child process that is a perfect clone of its parent.  Using  the fork before execvp ensures that the parent (shell) remains active after the child runs the command. Without fork, execvp would totally replace the shell process, terminating it after the command was completed. The fork enables process separation, allowing the shell to manage many commands while maintaining its own execution environment.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**: If the fork() fails it returns -1, and the parent process checks if pid is less than 0, and it is less then 0 the it prints an error (e.g., perror("fork")), and returns an error code (e.g., ERR_EXEC_CMD). This ensures that the shell remains responsive and does not crash even when system resources run out.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**: To locate the executable,the execvp() looks in the PATH environment variable. It scans the each and every directory in PATH in sequence to provided command.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**: Wait() stops the parent process until the child stops working, which prevents zombie processes (the late processes which remain after shutdown).  Without wait(), the shell would not know when the kid had finished, and the child's exit status would go unreported. It also enables the proper sequencing of command execution.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**: WEXITSTATUS(status) returns the exit code of the child process from the status integer. This is necessary for identifying whether the command succeeded (e.g., 0 for success, non-zero for errors) and propagating error codes to the shell's logic.

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**: The parser keeps track of quotes via the in_quote flag.  When a " is encountered, spaces inside quotations are treated as part of a single token.  This ensures that instructions such as echo "Hello World" are processed as a single argument (Hello World), rather than numerous tokens.  To handle arguments that contain spaces or unusual characters, we must quote them.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:
    > Part 1: To handle piped instructions, use command_list_t (e.g., cmd1 | cmd2).
    > 
    > Part 2: Simplified to cmd_buff_t for single commands and removed pipe support.
    > 
    > Challenges included ensuring that quoted arguments (e.g., echo "Hello World") were correctly tokenized without breaking extreme cases such as empty strings or unterminated quotes.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**: Signals are asynchronous notifications (for example, SIGINT for Ctrl+C) that manage process events. Unlike IPC (pipes/sockets), they are lightweight and do not carry data payloads.

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:
    > SIGINT: Request for graceful termination (which can be caught and cleaned up).
    > SIGTERM: Default termination signal (whcih enables cleanup).
    > SIGKILL: Causes immediate termination (which cannot be caught).


- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  It pauses the process (e.g., Ctrl+Z). Unlike SIGINT, it cannot be detected or disregarded because it is utilized for the job control in the shells.
