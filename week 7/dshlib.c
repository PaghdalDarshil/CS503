#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include "dshlib.h"

int last_return_code = 0;

int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX + 1);
    if (!cmd_buff->_cmd_buffer) 
        return ERR_MEMORY;
    cmd_buff->argc = 0;
    memset(cmd_buff->argv, 0, sizeof(cmd_buff->argv));
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
    }
    cmd_buff->_cmd_buffer = NULL;
    cmd_buff->argc = 0;
    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || !cmd_buff) return ERR_CMD_ARGS_BAD;

    if (strlen(cmd_line) > SH_CMD_MAX) {
        fprintf(stderr, "Error: Command too long.\n");
        last_return_code = 127;  // Ensure the test gets expected exit code
        return ERR_CMD_ARGS_BAD;
    }        

    char *input = cmd_line;
    int num_tokens = 0;
    int in_quote = 0;
    
    while (isspace(*input)) input++;
    
    if (*input == '\0') 
        return WARN_NO_CMDS;

    char *current = input;
    char *token_start = input;
    char *tokens[CMD_ARGV_MAX] = { NULL };

    while (*current) {
        if (*current == '"') {
            in_quote = !in_quote;
            if (!in_quote) {
                tokens[num_tokens] = strndup(token_start, current - token_start);
                if (++num_tokens >= CMD_ARGV_MAX) goto too_many_tokens;
                while (isspace(*++current)) current++;
                token_start = current;
                continue;
            }
            token_start = current + 1;
        }
        else if (!in_quote && isspace(*current)) {
            if (current > token_start) {
                tokens[num_tokens] = strndup(token_start, current - token_start);
                if (++num_tokens >= CMD_ARGV_MAX) goto too_many_tokens;
            }
            while (isspace(*current)) current++;
            token_start = current;
            continue;
        }
        current++;
    }

    if (current > token_start && !in_quote) {
        tokens[num_tokens] = strndup(token_start, current - token_start);
        if (++num_tokens >= CMD_ARGV_MAX) goto too_many_tokens;
    }

    if (in_quote) {
        for (int i = 0; i < num_tokens; i++) free(tokens[i]);
        return ERR_CMD_ARGS_BAD;
    }

    free_cmd_buff(cmd_buff);
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
    if (!cmd_buff->_cmd_buffer) {
        for (int i = 0; i < num_tokens; i++) free(tokens[i]);
        return ERR_MEMORY;
    }

    char *ptr = cmd_buff->_cmd_buffer;
    cmd_buff->argc = num_tokens;
    for (int i = 0; i < num_tokens; i++) {
        strcpy(ptr, tokens[i]);
        cmd_buff->argv[i] = ptr;
        ptr += strlen(tokens[i]) + 1;
        free(tokens[i]);
    }
    cmd_buff->argv[num_tokens] = NULL;
    return OK;

too_many_tokens:
    for (int i = 0; i < num_tokens; i++) free(tokens[i]);
    return ERR_TOO_MANY_COMMANDS;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (!cmd_line || !clist) return ERR_CMD_ARGS_BAD;

    clist->num = 0;
    char *token = strtok(cmd_line, PIPE_STRING);

    while (token != NULL) {
        if (clist->num >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }

        // Trim leading and trailing spaces
        while (isspace(*token)) token++;
        char *end = token + strlen(token) - 1;
        while (end > token && isspace(*end)) *end-- = '\0';

        build_cmd_buff(token, &clist->commands[clist->num]);
        clist->num++;

        token = strtok(NULL, PIPE_STRING);
    }

    return OK;
}

int execute_pipeline(command_list_t *clist) {
    int pipes[CMD_MAX - 1][2];
    pid_t pids[CMD_MAX];
    int exit_status = 0;  // Track failure status

    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe error");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("fork error");
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) {  // Child process
            if (i > 0) dup2(pipes[i - 1][0], STDIN_FILENO);
            if (i < clist->num - 1) dup2(pipes[i][1], STDOUT_FILENO);

            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("exec failed");
            _exit(127);  // Exit with error code 127
        }
    }

    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < clist->num; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            exit_status = WEXITSTATUS(status);
        }
    }

    return exit_status;
}

int exec_local_cmd_loop() {
    char input[SH_CMD_MAX];
    command_list_t cmd_list;

    while (1) {
        printf("%s", SH_PROMPT);
        if (!fgets(input, SH_CMD_MAX, stdin)) {
            printf("\n");
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) {
            fprintf(stderr, "%s\n", CMD_WARN_NO_CMD);
            last_return_code = 0;  // Ensure correct exit code
            continue;
        }                

        if (strcmp(input, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }

        // Handle built-in 'cd' command in parent process
        if (strncmp(input, "cd ", 3) == 0) {
            char *dir = input + 3;
            while (isspace(*dir)) dir++;
            if (chdir(dir) != 0) {
                perror("cd error");
            }
            continue;
        }

        int parse_status = build_cmd_list(input, &cmd_list);
        if (parse_status == WARN_NO_CMDS) {
            fprintf(stderr, "%s", CMD_WARN_NO_CMD);
            continue;
        } else if (parse_status == ERR_TOO_MANY_COMMANDS) {
            fprintf(stderr, "%s", CMD_ERR_PIPE_LIMIT);
            continue;
        }

        execute_pipeline(&cmd_list);
    }

    return OK;
}