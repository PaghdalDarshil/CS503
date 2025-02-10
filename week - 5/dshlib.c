#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */
static void trim_whitespace(char *str) {
    if (str == NULL) return;

    size_t len = strlen(str);
    if (len == 0) return;

    size_t start = 0;
    while (isspace((unsigned char)str[start])) {
        start++;
    }

    size_t end = len;
    while (end > start && isspace((unsigned char)str[end - 1])) {
        end--;
    }

    memmove(str, str + start, end - start);
    str[end - start] = '\0';
}
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
     clist->num = 0;

    if (cmd_line == NULL || strlen(cmd_line) == 0) {
        return WARN_NO_CMDS;
    }

    char *cmd_line_copy = strdup(cmd_line);
    if (cmd_line_copy == NULL) {
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    char *saveptr1;
    char *command_part;
    int num_commands = 0;
    command_part = strtok_r(cmd_line_copy, "|", &saveptr1);
    while (command_part != NULL) {
        trim_whitespace(command_part);
        if (strlen(command_part) > 0) {
            if (num_commands >= CMD_MAX) {
                free(cmd_line_copy);
                return ERR_TOO_MANY_COMMANDS;
            }
            char exe_buf[EXE_MAX] = {0};
            char args_buf[ARG_MAX] = {0};
            char *saveptr2;
            char *exe_token = strtok_r(command_part, " ", &saveptr2);
            if (exe_token == NULL) {
                command_part = strtok_r(NULL, "|", &saveptr1);
                continue;
            }
            if (strlen(exe_token) >= EXE_MAX) {
                free(cmd_line_copy);
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strncpy(exe_buf, exe_token, EXE_MAX - 1);
            exe_buf[EXE_MAX - 1] = '\0';
            char *arg_token;
            int args_len = 0;
            while ((arg_token = strtok_r(NULL, " ", &saveptr2)) != NULL) {
                size_t token_len = strlen(arg_token);

                if (args_len == 0) {
                    if (token_len >= ARG_MAX) {
                        free(cmd_line_copy);
                        return ERR_CMD_OR_ARGS_TOO_BIG;
                    }
                    strncpy(args_buf, arg_token, ARG_MAX - 1);
                    args_len = token_len;
                } else {
                    if (args_len + 1 + token_len >= ARG_MAX) {
                        free(cmd_line_copy);
                        return ERR_CMD_OR_ARGS_TOO_BIG;
                    }
                    strncat(args_buf, " ", ARG_MAX - args_len - 1);
                    strncat(args_buf, arg_token, ARG_MAX - args_len - 1);
                    args_len += 1 + token_len;
                }
            }
            args_buf[ARG_MAX - 1] = '\0';
            strncpy(clist->commands[num_commands].exe, exe_buf, EXE_MAX - 1);
            clist->commands[num_commands].exe[EXE_MAX - 1] = '\0';
            strncpy(clist->commands[num_commands].args, args_buf, ARG_MAX - 1);
            clist->commands[num_commands].args[ARG_MAX - 1] = '\0';
            num_commands++;
        }
        command_part = strtok_r(NULL, "|", &saveptr1);
    }
    free(cmd_line_copy);
    if (num_commands == 0) {
        return WARN_NO_CMDS;
    }
    clist->num = num_commands;
    return OK;
}