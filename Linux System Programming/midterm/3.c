#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MODE_EXEC (S_IXUSR | S_IXGRP | S_IXOTH)

int main(int argc, char *argv[]) {
	struct stat statbuf;
	int i;

	for (int i = 1; i < argc; i++) {
		lstat(argv[i], &statbuf);
		statbuf.st_mode |= MODE_EXEC;
		statbuf.st_mode ^= (MODE_EXEC ^ S_IXUSR);

		if (chmod(argv[i], statbuf.st_mode) < 0) fprintf(stderr, "%s : chmod error\n", argv[i]);
		else printf("%s : file permission was changed\n", argv[i]);
	}

	exit(0);
}
