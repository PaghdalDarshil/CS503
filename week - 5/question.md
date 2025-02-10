1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  fgets() is an excellent choice because it safely receives user input while preventing the buffer overflows. Unlike gets(), which is unsafe because it lacks a size restriction, fgets() assures that input is limited to a specific buffer size (SH_CMD_MAX in our case). And it also supports multi-word input and captures newlines, making it easier to interpret shell commands.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  Malloc() enables dynamic memory allocation, which is critical for the flexibility. When we usees a fixed-size array, the memory is allocated at build time, which might be inefficient if the buffer is either too large (wasting memory) or too tiny (causing truncation). malloc() ensures that we allocate only enough memory at runtime, avoiding the excessive memory usage while allowing for longer commands.

3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  Trimming leading and trailing spaces is required to avoid unexpected behavior when processing the commands. If the spaces are not removed then we might have some issues:
  1. Command Parsing Issues - Extra spaces may cause erroneous parsing, leading the shell to misinterpret the arguments.
  2. Execution Errors - Some commands may fail if the extra spaces are given as inputs.
  3. undesired Commands - Typing " ls " instead of "ls" may result in undesired behavior.
By reducing spaces, we ensure that commands are executed accurately and without ambiguity.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:
  1. Output Redirection (> and >>):-
   ls > output.txt → Redirects ls output to output.txt, overwriting it.
   ls >> Appends ls output to output.txt.
   The challenge is to Manage the file permissions and ensuring that the file is opened correctly.
  2. Input Redirection (<):
   sort < data.txt → Reads input from data.txt rather than the keyboard.
   The challenge is to handle the errors when files do not exist or do not have read permissions.
  3. Error Redirection (2>):
   gcc program.c. Redirect compiler problems to error.log.
   The challenge is to Differentiate between STDOUT and STDERR while keeping the error messages meaningful to the user.


- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:
  1. Redirection (> and <): Changes the command's input or output source. It usually works with the files (for example, command > file.txt saves output to a file).
  2. Piping (|) is used to pass the output of one command as input to another (command1 | command2). It allows you to link numerous commands together in a succession.
   
  Redirection is commonly used for the file-based input/output management, whereas piping is for the command-to-command communication.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  The separation of STDERR (error messages) and STDOUT (normal output) guarantees that errors do not interfere with desired results. This is important for:
  1. Debugging - Separating the errors makes them easier to diagnose.
  2. Automation - Scripts that process STDOUT should not be interrupted by the error messages.
  3. User Experience - Keeping the error messages distinct allows consumers to better grasp issues.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  Our shell should handle problems by validating command exit codes and printing any relevant error messages. When both STDOUT and STDERR need to be combined, users can specify it with 2>&1, which redirects STDERR to STDOUT.
