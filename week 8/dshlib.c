#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"
#include <errno.h>
#include <limits.h>
#include <sys/types.h>  
#include <sys/stat.h>    

int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;

    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX);
    if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;

    memset(cmd_buff->_cmd_buffer, 0, SH_CMD_MAX);
    memset(cmd_buff->argv, 0, sizeof(cmd_buff->argv));
    cmd_buff->argc = 0;
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_mode = false;

    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff || !cmd_buff->_cmd_buffer) return ERR_MEMORY;
    free(cmd_buff->_cmd_buffer);
    cmd_buff->_cmd_buffer = NULL; 
    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || !cmd_buff) return ERR_CMD_ARGS_BAD;

    if (!cmd_buff->_cmd_buffer) {
        cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
        if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;
    }

    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0'; 

    cmd_buff->argc = 0;
    char *token = strtok(cmd_buff->_cmd_buffer, " \t");
    while (token != NULL) {
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1) {
            fprintf(stderr, "error: too many arguments\n");
            return ERR_CMD_ARGS_BAD;
        }
        cmd_buff->argv[cmd_buff->argc++] = token;
        token = strtok(NULL, " \t");
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
        free_cmd_buff(cmd);
        exit(0);
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        char target_dir[PATH_MAX];

        if (cmd->argc > 1) {
            strncpy(target_dir, cmd->argv[1], PATH_MAX);
        } else {
            strncpy(target_dir, getenv("HOME"), PATH_MAX);
        }

        target_dir[PATH_MAX - 1] = '\0';  

        if (chdir(target_dir) != 0) {
            perror("cd failed");
        }
        return BI_EXECUTED;
    }

    return BI_NOT_BI;
}

Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, "exit") == 0) return BI_CMD_EXIT;
    if (strcmp(input, "cd") == 0) return BI_CMD_CD;
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
            if (i > 0) {
                dup2(prev_pipe, STDIN_FILENO);
                close(prev_pipe);
            }
            if (i < num_cmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp failed");
            exit(ERR_EXEC_CMD);
        } else { // Parent process
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

        command_list_t clist;
        ret = build_cmd_list(cmd_line, &clist);
        if (ret < 0) {
            if (ret == WARN_NO_CMDS) printf(CMD_WARN_NO_CMD);
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
