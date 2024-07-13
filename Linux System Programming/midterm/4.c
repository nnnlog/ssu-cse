#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define NAME_SIZE 256

int main(int argc, char *argv[]) {
	FILE *fp;
	char fname[NAME_SIZE];
	struct stat statbuf;

	printf("Enter your file name: ");
	scanf("%s", fname);

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	lstat(fname, &statbuf);

	printf("File Type: ");
	if (S_ISREG(statbuf.st_mode)) printf("Regular file");
	else if (S_ISDIR(statbuf.st_mode)) printf("Directory");
	else printf("Symbolic link");

	printf("\nOwner Permission:\n");
	if (statbuf.st_mode & S_IRUSR) printf("Read Permisson bit set\n");
	if (statbuf.st_mode & S_IWUSR) printf("Write Permisson bit set\n");
	if (statbuf.st_mode & S_IXUSR) printf("Execute Permisson bit set\n");

	printf("File Size: %ld bytes\n", statbuf.st_size);

	printf("Last Modification Time: %s", ctime(&statbuf.st_mtime));

	fclose(fp);

	exit(0);
}
