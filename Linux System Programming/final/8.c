#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>

#define BUFMAX 4096

char *deleteSpace(char *s) {
	char *ret = malloc(strlen(s) + 1);
	char *ptr = ret;
	for (char *i = s; *i; i++) if (*i != ' ') *ptr++ = *i;
	*ptr = 0;
	strcpy(s, ret);
	free(ret);
	return s;
}

int main(int argc, char *argv[]) {
	int fd1, fd2;
	char buf1[BUFMAX], buf2[BUFMAX];
	int len;
	char *answer, *student;
	int result = 0;

	if (argc != 3) {
		fprintf(stderr, "Usage : %s <ANS_FILE> <STU_FILE>\n", argv[0]);
		exit(1);
	}

	if ((fd1 = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s.\n", argv[1]);
		exit(1);
	}
	if ((fd2 = open(argv[2], O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s.\n", argv[2]);
		exit(1);
	}

	int a = read(fd1, buf1, BUFMAX);
	buf1[a] = 0;
	int b = read(fd2, buf2, BUFMAX);
	buf2[b] = 0;
	deleteSpace(buf2);

	int f = 0;

	strcat(buf1, ":");
	char tmp[BUFMAX];
	char *ptr = tmp;
	for (int i = 0; i <= a; i++) {
		if (buf1[i] == ':') {
			*ptr = 0;
			deleteSpace(tmp);
//			printf("%s %s\n", tmp, buf2);
			if (!strcmp(tmp, buf2)) {
				printf("RIGHT\n");
				f = 1;
				break;
			}
			*tmp = 0;
			ptr = tmp;
		} else {
			*ptr++ = buf1[i];
		}
	}

	if (!f) printf("WRONG\n");

	close(fd1);
	close(fd2);
	
	exit(0);
}
