#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include "commons/path.h"
#include "commons/const.h"

int main(int argc, char *argv[]) {
    set_additional_path(argv[0]);

    while (1) {
        printf("20221494> ");

        char buf[BUFFER_MAX] = {0};
        if (scanf("%[^\n]", buf) == EOF) break;

        getchar();
        if (!*buf) continue; // simply type "Enter"

        strcat(buf, " ");

        int cmd_argc = 0;
        char **cmd_argv = NULL;
        char *ptr = buf;

        while ((ptr = strtok(ptr, " ")) != NULL) {
            if (++cmd_argc == 1) cmd_argv = malloc(sizeof(char **));
            else cmd_argv = realloc(cmd_argv, cmd_argc * sizeof(char **));

            cmd_argv[cmd_argc - 1] = ptr;
            ptr = NULL;
        }
        cmd_argv = realloc(cmd_argv, (cmd_argc + 1) * sizeof(char **));
        cmd_argv[cmd_argc] = 0; // execv로 하려면 마지막이 null이어야 함

        int execute_child_process = 0;
        char *cmd = *cmd_argv;
        if (!strcmp(cmd, "add")) {
            cmd_argv[0] = "command_add";
            execute_child_process = 1;
        } else if (!strcmp(cmd, "delete")) {
            cmd_argv[0] = "command_delete";
            execute_child_process = 1;
        } else if (!strcmp(cmd, "tree")) {
            cmd_argv[0] = "command_tree";
            execute_child_process = 1;
        } else if (!strcmp(cmd, "help")) {
            cmd_argv[0] = "command_help";
            execute_child_process = 1;
        } else if (!strcmp(cmd, "exit")) {
            exit(0);
        } else {
            // unknown command
            cmd_argv[0] = "command_help";
            execute_child_process = 1;
        }

        if (execute_child_process) { // 하위 프로세스 생성해야 한다면
            if (!fork()) {
                execvp(*cmd_argv, cmd_argv); // fork 후 exec
            } else {
                int s;
                wait(&s);
            }
        }

        free(cmd_argv);
        cmd_argv = NULL;
    }
}
