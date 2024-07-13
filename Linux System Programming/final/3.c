#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define NAME_SIZE 256

int main(int argc, char *argv[]) {
	FILE *fp1, *fp2, *fp3;
	char buf1[NAME_SIZE], buf2[NAME_SIZE];
	char fname1[NAME_SIZE], fname2[NAME_SIZE], temp[] = "temp.txt";

	printf("Enter your first file name: ");
	scanf("%s", fname1);
	printf("Enter your second file name: ");
	scanf("%s", fname2);

	if ((fp1 = fopen(fname1, "r")) == NULL || (fp2 = fopen(fname2, "r")) == NULL || (fp3 = fopen(temp, "w")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	do {
		*buf1 = *buf2 = 0;
		fgets(buf1, NAME_SIZE, fp1);
		fgets(buf2, NAME_SIZE, fp2);
		fputs(buf1, fp3);
		fputs(buf2, fp3);
	} while (*buf1 || *buf2);

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	remove(fname1);
//	printf("%d %d %s\n", t, errno, "");
	remove(fname2);
	rename(temp, fname1);

	exit(0);
}
