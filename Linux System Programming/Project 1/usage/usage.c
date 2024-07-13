#include "usage.h"

#include <stdio.h>

void print_help_program() {
    printf("Usage: ssu_backup <md5 | sha1>\n");
}

void print_help_all() {
    printf("Usage:\n"
           " > add [FILENAME] [OPTION]\n"
           " -d : add directory recursive\n"
           " > remove [FILENAME] [OPTION]\n"
           " -a : remove all file(recursive)\n"
           " -c : clear backup directory\n"
           " > recover [FILENAME] [OPTION]\n"
           " -d : recover directory recursive\n"
           " -n [NEWNAME] : recover file with new name\n"
           " > ls\n"
           " > vi\n"
           " > vim\n"
           " > help\n"
           " > exit\n");
}

void print_help_add() {
    printf("Usage:\n"
           " > add <FILENAME> [OPTION]\n"
           " -d : add directory recursive\n");
}

void print_help_remove() {
    printf("Usage:\n"
           " > remove [FILENAME] [OPTION]\n"
           " -a : remove all file(recursive)\n"
           " -c : clear backup directory\n");
}

void print_help_recover() {
    printf("Usage:\n"
           " > recover [FILENAME] [OPTION]\n"
           " -d : recover directory recursive\n"
           " -n [NEWNAME] : recover file with new name\n");
}
