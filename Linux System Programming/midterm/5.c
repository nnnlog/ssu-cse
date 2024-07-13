#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define NAME_SIZE 256

void append(FILE *src, FILE *dst) {
	char buf[4096];
	while (fgets(buf, 4096, src)) fputs(buf, dst);
}

int main(int argc, char *argv[]) {
	FILE *fp1, *fp2, *fp3;
	char fname1[NAME_SIZE], fname2[NAME_SIZE], output[NAME_SIZE];

	printf("Enter your first file name: ");
	fgets(fname1, NAME_SIZE, stdin);
	fname1[strlen(fname1) - 1] = 0;
	printf("Enter your second file name: ");
	fgets(fname2, NAME_SIZE, stdin);
	fname2[strlen(fname2) - 1] = 0;
	printf("Enter your destination file name: ");
	fgets(output, NAME_SIZE, stdin);
	output[strlen(output) - 1] = 0;

	if (!(fp1 = fopen(fname1, "r")) || !(fp2 = fopen(fname2, "r")) || !(fp3 = fopen(output, "w"))) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	append(fp1, fp3);
	append(fp2, fp3);

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	exit(0);
}
