#include "usage.h"

#include <stdio.h>

void print_usage() {
    printf("Usage:\n"
           " > add <DIRPATH> [OPTION]\n"
           " -t <TIME> : set repeat time\n"
           " > delete <DAEMON_PID>\n"
           " > tree <DIRPATH>\n"
           " > help\n"
           " > exit\n");
}

void print_usage_add() {
    printf("Usage:\n"
           " > add <DIRPATH> [OPTION]\n"
           " -t <TIME> : set repeat time\n");
}

void print_usage_delete() {
    printf("Usage:\n"
           " > delete <DAEMON_PID>\n");
}

void print_usage_tree() {
    printf("Usage:\n"
           " > tree <DIRPATH>\n");
}
