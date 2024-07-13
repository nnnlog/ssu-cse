#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <wait.h>

#define BUFFER_SIZE 1024
#define PATHMAX 4096

char *cmdlist[5] = {"add", "remove", "recover", "exit"};
char **arglist;

int prompt(char *exepath) {
	while (1) {
		printf("> ");
		char input[BUFFER_SIZE];
		if (fgets(input, BUFFER_SIZE, stdin) == NULL) break;
		input[strlen(input) - 1] = 0;
		strcat(input, " ");
		int len = strlen(input);
		int cnt = 0;
		char name[BUFFER_SIZE] = {0};
		for (int i = 0; i < len; i++) if (input[i] == ' ') {
			input[i] = 0;
			if (!*name) strcpy(name, input);//, printf("%d\n", i);
			input[i] = ' ';
			cnt++;
		}
	
		int cmd_cnt[] = {2, 2, 3, 1};
		int idx = -1;
		for (int i = 0; i < 4; i++) if (!strcmp(cmdlist[i], name)) idx = i;

		if (idx == -1) {
			printf("wrong command : %s\n", name);
			continue;
		}

		if (cmd_cnt[idx] != cnt) {
			if (idx == 0) printf("Usage : add <FILENAME>\n");
			else if (idx == 1) printf("Usage : remove <FILENAME>\n");
			else if (idx == 2) printf("Usage : recover <FILENAME> <NEWNAME>\n");
			else printf("Usage : exit\n");
			continue;
		}

		if (!strcmp(name, "exit")) {
			printf("program exit(0)...\n");
			exit(0);
		}

		arglist = malloc(sizeof(char*) * (cnt + 2));
		int curr_idx = 0;
		arglist[0] = exepath;
		for (int i = 1; i <= cnt; i++) {
			arglist[i] = input + curr_idx;
			for (; input[curr_idx] != ' '; curr_idx++);
			input[curr_idx++] = 0;
		}
		arglist[cnt + 1] = 0;

// 		for (int i = 0; i < cnt + 2; i++) {
// 			printf("%s\n", (i == cnt + 1) ? "NULL" : arglist[i]);
// 		}

		pid_t pid;
		if ((pid = fork()) < 0) {
			fprintf(stderr, "error fork\n");
			exit(1);
		} else if (pid == 0) {
			execv(exepath, arglist);
			exit(0);
		} else {
			int s;
			wait(&s);
		}
	}
}

void ac_chk(char *a) {
	if (access(a, F_OK)) {
		fprintf(stderr, "access error for %s\n", a);
		exit(1);
	}
}

void add(char *a) {
	ac_chk(a);
	char buf[PATHMAX];
	realpath(a, buf);
	printf("(%s) added.\n", buf);
}

void remove_(char *a) {
	ac_chk(a);
	char buf[PATHMAX];
	realpath(a, buf);
	printf("(%s) removed.\n", buf);
}

void recover(char *a, char *b) {
	ac_chk(a);
	ac_chk(b);
	char buf[PATHMAX];
	char buf2[PATHMAX];
	realpath(a, buf);
	realpath(b, buf2);
	printf("(%s) recover to (%s).\n", buf, buf2);
}

int main(int argc, char *argv[]) {
	if (argc >= 2) {
		if (!strcmp(argv[1], "add")) add(argv[2]);
		if (!strcmp(argv[1], "remove")) remove_(argv[2]);
		if (!strcmp(argv[1], "recover")) recover(argv[2], argv[3]);
	}

	prompt(argv[0]);
	exit(0);
}
