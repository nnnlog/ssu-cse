#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define TABLE_SIZE 128
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
	static struct {
		long offset;
		int length
	} table [TABLE_SIZE];
	char buf[BUFFER_SIZE];

	int length;
	int fd;

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "open error\n");
		exit(1);
	}

	int glob = 0, idx = 0;
	while ((length = read(fd, buf, BUFFER_SIZE)) > 0) {
		for (int i = 0; i < length; i++) {
			glob++;
			table[idx].length++;
			if (buf[i] == '\n') {
				table[++idx].offset = glob;
			}
		}
	}

	while (1) {
		printf("Enter line number : ");
		int i;
		scanf("%d", &i);
		--i;
		if (i < 0 || i >= idx) break;
		lseek(fd, table[i].offset, SEEK_SET);
		int sz = read(fd, buf, table[i].length);
		buf[sz] = 0;
		printf("%s", buf);
	}

	close(fd);
	exit(0);
}
