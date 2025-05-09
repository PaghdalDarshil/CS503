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

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */

extern void print_dragon();

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
        fprintf(stderr, "The command does not found\n");
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

    size_t buffer_size = 0;
    for (int i = 0; i < num_tokens; i++) {
        buffer_size += strlen(tokens[i]) + 1;
    }

    free_cmd_buff(cmd_buff);
    cmd_buff->_cmd_buffer = malloc(buffer_size);
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

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (!cmd->argv[0]) 
        return BI_NOT_BI;

    if (strcmp(cmd->argv[0], "exit") == 0) {
        return BI_CMD_EXIT;
    }
    else if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc < 2) {
            fprintf(stderr, "cd error: Missing argument\n");
            return BI_CMD_CD;
        }
        if (chdir(cmd->argv[1]) != 0) {
            perror("cd error");
        }
        return BI_CMD_CD;
    }
    else if (strcmp(cmd->argv[0], "dragon") == 0) {
        print_dragon();
        return BI_CMD_DRAGON;
    }
    else if (strcmp(cmd->argv[0], "rc") == 0) { 
        printf("%d\n", last_return_code);
        return BI_RC;
    }
    return BI_NOT_BI;
}

int exec_cmd(cmd_buff_t *cmd) {
    if (!cmd->argv[0]) return ERR_EXEC_CMD;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        return ERR_EXEC_CMD;
    } 
    else if (pid == 0) {  
        execvp(cmd->argv[0], cmd->argv);

        fprintf(stderr, "The command does not found\n");  
        exit(errno);
    }
    else {  
        int status;
        waitpid(pid, &status, 0);
        last_return_code = WEXITSTATUS(status);
        return last_return_code;
    }
}

int exec_local_cmd_loop() {
    char input[SH_CMD_MAX];
    cmd_buff_t cmd;

    if (alloc_cmd_buff(&cmd) != OK) {
        fprintf(stderr, "Failed to allocate command buffer\n");
        return ERR_MEMORY;
    }

    while (1) {
        printf("%s", SH_PROMPT);
        if (!fgets(input, SH_CMD_MAX, stdin)) {
            printf("\n");
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) {
            fprintf(stderr, "%s", CMD_WARN_NO_CMD);
            continue;
        }

        int parse_status = build_cmd_buff(input, &cmd);
        if (parse_status == WARN_NO_CMDS) {
            fprintf(stderr, "%s", CMD_WARN_NO_CMD);
            continue;
        } else if (parse_status == ERR_TOO_MANY_COMMANDS) {
            fprintf(stderr, "%s", CMD_ERR_PIPE_LIMIT);
            continue;
        }

        Built_In_Cmds bic = exec_built_in_cmd(&cmd);
        if (bic == BI_CMD_EXIT) {
            free_cmd_buff(&cmd);
            break;
        }
        else if (bic == BI_NOT_BI) {
            last_return_code = exec_cmd(&cmd);
            if (last_return_code != OK) {
                fprintf(stderr, "The command exited with status %d\n", last_return_code);
            }
        }

        free_cmd_buff(&cmd);
    }

    free_cmd_buff(&cmd);
    return OK;
}
