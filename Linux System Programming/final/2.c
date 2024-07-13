#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

int main() {
	int fd;
	int val;

	if ((fd = open("ssu_test.txt", O_RDONLY)) < 0) {
		fprintf(stderr, "open error\n");
		exit(1);
	}

	val = fcntl(fd, F_GETFD);
	if (val & FD_CLOEXEC) printf("close-on-exec bit on\n");
	else printf("close-on-exec bit off\n");

	fcntl(fd, F_SETFD, val | FD_CLOEXEC);
	val = fcntl(fd, F_GETFD);
	if (val & FD_CLOEXEC) printf("close-on-exec bit on\n");
	else printf("close-on-exec bit off\n");

	execl("./ssu_loop", "./ssu_loop", NULL);

	exit(0);
}
