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

int last_return_code = 0;

int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX);
    if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;
    memset(cmd_buff, 0, sizeof(cmd_buff_t));
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    memset(cmd_buff, 0, sizeof(cmd_buff_t));
    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    char *ptr = cmd_line;
    int in_quotes = 0;
    char *token_start = NULL;

    while (*ptr) {
        while (isspace(*ptr) && !in_quotes) ptr++;
        if (*ptr == '"') {
            in_quotes = !in_quotes;
            ptr++;
            token_start = ptr;
        } else {
            token_start = ptr;
        }

        while (*ptr && (in_quotes || !isspace(*ptr))) {
            if (*ptr == '"') {
                in_quotes = !in_quotes;
                *ptr = '\0';
            }
            ptr++;
        }

        if (*ptr) {
            *ptr = '\0';
            ptr++;
        }

        cmd_buff->argv[cmd_buff->argc++] = token_start;
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1) {
            fprintf(stderr, "error: too many arguments\n");
            return ERR_CMD_ARGS_BAD;
        }
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    clist->num = 0;
    char *saveptr;
    char *token = strtok_r(cmd_line, "|", &saveptr);

    while (token != NULL) {
        if (clist->num >= CMD_MAX) {
            fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX);
            return ERR_TOO_MANY_COMMANDS;
        }

        while (*token == ' ') token++;
        size_t len = strlen(token);
        while (len > 0 && token[len - 1] == ' ') token[--len] = '\0';

        cmd_buff_t *cmd = &clist->commands[clist->num];
        if (alloc_cmd_buff(cmd) != OK) return ERR_MEMORY;

        int rc = build_cmd_buff(token, cmd);
        if (rc != OK) {
            free_cmd_buff(cmd);
            return rc;
        }

        clist->num++;
        token = strtok_r(NULL, "|", &saveptr);
    }

    if (clist->num == 0) {
        printf(CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }
    return OK;
}

int free_cmd_list(command_list_t *clist) {
    for (int i = 0; i < clist->num; i++) {
        free_cmd_buff(&clist->commands[i]);
    }
    clist->num = 0;
    return OK;
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (strcmp(cmd->argv[0], "exit") == 0) {
        printf("exiting...\n");
        fflush(stdout); 
        free_cmd_buff(cmd);
        exit(0);
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc > 2) {
            fprintf(stderr, "error: cd command accepts at most 1 argument\n");
            return ERR_CMD_ARGS_BAD;
        }
        if (chdir(cmd->argv[1]) != 0) {
            perror("cd failed");
        }
        return BI_EXECUTED;
    }

    return BI_NOT_BI;
}

int execute_pipeline(command_list_t *clist) {
    int num_cmds = clist->num;
    int prev_pipe = -1;
    int fd[2];
    pid_t pids[num_cmds];

    for (int i = 0; i < num_cmds; i++) {
        if (i < num_cmds - 1 && pipe(fd) < 0) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) {
            int out_fd = -1; 
            char *argv_clean[CMD_ARGV_MAX];
            int j = 0;

            for (int k = 0; k < clist->commands[i].argc; k++) {
                if (strcmp(clist->commands[i].argv[k], ">") == 0) {
                    if (k + 1 < clist->commands[i].argc) {
                        out_fd = open(clist->commands[i].argv[k + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (out_fd < 0) {
                            perror("Error opening output file");
                            exit(ERR_EXEC_CMD);
                        }
                        dup2(out_fd, STDOUT_FILENO);
                        close(out_fd);
                        k++; 
                    }
                } else if (strcmp(clist->commands[i].argv[k], ">>") == 0) {
                    if (k + 1 < clist->commands[i].argc) {
                        out_fd = open(clist->commands[i].argv[k + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                        if (out_fd < 0) {
                            perror("Error opening output file");
                            exit(ERR_EXEC_CMD);
                        }
                        dup2(out_fd, STDOUT_FILENO);
                        close(out_fd);
                        k++; 
                    }
                } else {
                    argv_clean[j++] = clist->commands[i].argv[k];
                }
            }
            argv_clean[j] = NULL; 

            if (i > 0) {
                dup2(prev_pipe, STDIN_FILENO);
                close(prev_pipe);
            }
            if (i < num_cmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            
            execvp(argv_clean[0], argv_clean);
            perror("execvp failed");
            exit(ERR_EXEC_CMD);
        } else { // Parent Process
            if (i > 0) close(prev_pipe);
            if (i < num_cmds - 1) {
                prev_pipe = fd[0];
                close(fd[1]);
            }
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }
    return OK;
}

int exec_local_cmd_loop() {
    char input_buffer[SH_CMD_MAX];
    command_list_t clist;

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(input_buffer, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        input_buffer[strcspn(input_buffer, "\n")] = '\0';
        if (strlen(input_buffer) == 0) {
            printf(CMD_WARN_NO_CMD);
            printf("\n"); 
            continue;
        }

        int rc = build_cmd_list(input_buffer, &clist);
        if (rc != OK) {
            if (rc == WARN_NO_CMDS) printf(CMD_WARN_NO_CMD);
            else if (rc == ERR_TOO_MANY_COMMANDS) printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }

        if (clist.num == 1 && exec_built_in_cmd(&clist.commands[0]) == BI_EXECUTED) {
            free_cmd_list(&clist);
            continue;
        }

        execute_pipeline(&clist);
        free_cmd_list(&clist);
    }
    return OK;
}
