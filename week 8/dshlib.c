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
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = NULL;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = NULL;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || !cmd_buff) return ERR_CMD_ARGS_BAD;

    if (strlen(cmd_line) > SH_CMD_MAX) {
        fprintf(stderr, "error: command is too long\n");
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }
    if (!cmd_buff->_cmd_buffer) {
        cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
        if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;
    }
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    cmd_buff->argc = 0;
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    char *token = strtok(cmd_buff->_cmd_buffer, " \t");
    while (token != NULL) {
        if (strcmp(token, "<") == 0) { 
            token = strtok(NULL, " \t");
            if (token) {
                cmd_buff->input_file = strdup(token);  
            } else {
                fprintf(stderr, "error: missing the input file\n");
                return ERR_CMD_ARGS_BAD;
            }
        } else if (strcmp(token, ">") == 0) {  
            token = strtok(NULL, " \t");
            if (token) {
                cmd_buff->output_file = strdup(token); 
            } else {
                fprintf(stderr, "error: missing the output file\n");
                return ERR_CMD_ARGS_BAD;
            }
        } else {  
            if (cmd_buff->argc >= CMD_ARGV_MAX - 1) {
                fprintf(stderr, "error: too many arguments\n");
                return ERR_TOO_MANY_COMMANDS;
            }
            cmd_buff->argv[cmd_buff->argc] = token;
            cmd_buff->argc++;
        }
        token = strtok(NULL, " \t");
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    if (cmd_buff->argc == 0) {
        return WARN_NO_CMDS;
    }
    return OK;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    clist->num = 0;
    char *cmd_copy = strdup(cmd_line);
    if (!cmd_copy) {
         return ERR_MEMORY;
    }
    char *token = strtok(cmd_copy, PIPE_STRING);
    while(token != NULL && clist->num < CMD_MAX) {
        
         while(*token && isspace(*token)) token++;
         if (*token == '\0') {
             token = strtok(NULL, PIPE_STRING);
             continue;
         }
         int ret = build_cmd_buff(token, &clist->commands[clist->num]);
         if (ret < 0) {
             free(cmd_copy);
             return ret;
         }
         clist->num++;
         token = strtok(NULL, PIPE_STRING);
    }
    free(cmd_copy);
    if (clist->num == 0) {
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

int execute_pipeline(command_list_t *clist) {
    int num_cmds = clist->num;
    int prev_fd = -1;
    pid_t pids[CMD_MAX];
    int pipefd[2];
    for (int i = 0; i < num_cmds; i++) {
        if (i < num_cmds - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                return ERR_EXEC_CMD;
            }
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        }
        if (pid == 0) {  
            if (clist->commands[i].input_file) {
                int fd = open(clist->commands[i].input_file, O_RDONLY);
                if (fd < 0) {
                    perror("open failed for the input redirection");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (clist->commands[i].output_file) {
                int fd = open(clist->commands[i].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("open failed for the output redirection");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            if (i > 0) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < num_cmds - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        } else { 
            if (i < num_cmds - 1) {
                close(pipefd[1]);
            }

            if (i > 0) {
                close(prev_fd);
            }

            if (i < num_cmds - 1) {
                prev_fd = pipefd[0];
            }
            pids[i] = pid;
        }
    }
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }
    return OK;
}

int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "r");
    if (!in) return -1;
    FILE *out = fopen(dst, "w");
    if (!out) { fclose(in); return -1; }
    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        fwrite(buffer, 1, n, out);
    }
    fclose(in);
    fclose(out);
    return 0;
}

int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    int ret;

    char temp_dir[] = "/tmp/dsh_envXXXXXX";
    if (mkdtemp(temp_dir) == NULL) {
        perror("mkdtemp");
        return ERR_MEMORY;
    }
    
    char dest_file[256];
    snprintf(dest_file, sizeof(dest_file), "%s/dshlib.c", temp_dir);
    if (copy_file("dshlib.c", dest_file) != 0) {
        perror("copy_file");
        return ERR_MEMORY;
    }
    
    if (chdir(temp_dir) != 0) {
        perror("chdir");
        return ERR_MEMORY;
    }
    
    while (1) {
        printf("%s", SH_PROMPT);  
        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL) {
            printf("\n");
            break;
        }
        
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }

        char *temp = cmd_line;
        while (*temp && isspace(*temp)) temp++;
        if (*temp == '\0') {
            printf(CMD_WARN_NO_CMD);
            continue;
        }
        command_list_t clist;
        ret = build_cmd_list(cmd_line, &clist);
        if (ret < 0) {
            if (ret == WARN_NO_CMDS) {
                printf(CMD_WARN_NO_CMD);
            } else if (ret == ERR_TOO_MANY_COMMANDS) {
                printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
                printf("\n");
            } else if (ret == ERR_MEMORY) {
                perror("Memory error");
            }
            continue;
        }
        ret = execute_pipeline(&clist);
        if (ret < 0) {
            perror("Execution error");
        }
        free_cmd_list(&clist);
    }
    return OK;
}
