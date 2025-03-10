#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "dshlib.h"

int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX);
    if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;  
    memset(cmd_buff->argv, 0, sizeof(cmd_buff->argv));
    cmd_buff->argc = 0;
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
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
        while (*token == ' ') token++;

        size_t len = strlen(token);
        while (len > 0 && token[len - 1] == ' ') token[--len] = '\0';

        if (clist->num >= CMD_MAX) {
            fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX);
            return ERR_TOO_MANY_COMMANDS;
        }

        int rc = build_cmd_buff(token, &clist->commands[clist->num]);
        if (rc != OK) {
            free_cmd_list(clist);
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

int free_cmd_list(command_list_t *cmd_lst) {
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    return OK;
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (strcmp(cmd->argv[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        char *target_dir = NULL;
        if (cmd->argc == 1 || cmd->argv[1] == NULL) {
            target_dir = getenv("HOME");
            if (!target_dir) {
                fprintf(stderr, "error: HOME environment variable not set\n");
                return ERR_CMD_ARGS_BAD;
            }
        } 
        else if (cmd->argc == 2) {
            target_dir = cmd->argv[1];
        } 
        else {
            fprintf(stderr, "error: cd command accepts at most 1 argument\n");
            return ERR_CMD_ARGS_BAD;
        }
        if (chdir(target_dir) != 0) {
            perror("cd failed");
        }
        return BI_EXECUTED;
    }

    if (strcmp(cmd->argv[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd failed");
        }
        return BI_EXECUTED;
    }

    if (strcmp(cmd->argv[0], "echo") == 0) {
        for (int i = 1; i < cmd->argc; i++) {
            printf("%s ", cmd->argv[i]);
        }
        printf("\n");
        return BI_EXECUTED;
    }

    return BI_NOT_BI;
}

Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, "exit") == 0) return BI_CMD_EXIT;
    if (strcmp(input, "cd") == 0) return BI_CMD_CD;
    if (strcmp(input, "rc") == 0) return BI_CMD_RC;
    if (strcmp(input, "stop-server") == 0) return BI_CMD_STOP_SVR;
    return BI_NOT_BI;
}

int execute_pipeline(command_list_t *clist) {
    int num_cmds = clist->num;
    int pipes[2 * (num_cmds - 1)]; 
    pid_t pids[num_cmds];

    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes + i * 2) < 0) {
            perror("pipe failed");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork failed");
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) { 
            if (i > 0) {
                dup2(pipes[(i - 1) * 2], STDIN_FILENO);
            }
            if (i < num_cmds - 1) {
                dup2(pipes[i * 2 + 1], STDOUT_FILENO);
            }
            for (int j = 0; j < 2 * (num_cmds - 1); j++) {
                close(pipes[j]);
            }
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp failed");
            exit(ERR_EXEC_CMD);
        }
    }

    for (int i = 0; i < 2 * (num_cmds - 1); i++) {
        close(pipes[i]);
    }

    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return OK;
}

int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    int ret;

    while (1) {
        printf("%s", SH_PROMPT);

        if (!fgets(cmd_line, sizeof(cmd_line), stdin)) {
            printf("\n");
            break;
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        if (strlen(cmd_line) == 0) continue;

        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            printf("Exiting...\n");
            break;
        }

        command_list_t clist;
        ret = build_cmd_list(cmd_line, &clist);
        if (ret < 0) {
            if (ret == WARN_NO_CMDS) printf(CMD_WARN_NO_CMD);
            continue;
        }

        if (clist.num > 1) {
            for (int i = 0; i < clist.num; i++) {
                if (match_command(clist.commands[i].argv[0]) != BI_NOT_BI) {
                    fprintf(stderr, "error: built-in command in pipeline\n");
                    free_cmd_list(&clist);
                    continue;
                }
            }
        } else {
            Built_In_Cmds bi = match_command(clist.commands[0].argv[0]);
            if (bi != BI_NOT_BI) {
                exec_built_in_cmd(&clist.commands[0]);
                free_cmd_list(&clist);
                continue;
            }
        }

        execute_pipeline(&clist);
        free_cmd_list(&clist);
    }
    return OK;
}
