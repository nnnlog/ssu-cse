#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define NAME_SIZE 256

int main(int argc, char *argv[]) {
	FILE *fp1, *fp2, *fp3;
	char fname1[NAME_SIZE], fname2[NAME_SIZE], output[NAME_SIZE];
	int ch;

	printf("Enter your first file name: ");
	fgets(fname1, NAME_SIZE, stdin);

	printf("Enter your second file name: ");
	fgets(fname2, NAME_SIZE, stdin);

	printf("Enter your destination file name: ");
	fgets(output, NAME_SIZE, stdin);

	fname1[strlen(fname1) - 1] = 0;
	fname2[strlen(fname2) - 1] = 0;
	output[strlen(output) - 1] = 0;

	if ((fp1 = fopen(fname1, "r")) == NULL || (fp2 = fopen(fname2, "r")) == NULL || (fp3 = fopen(output, "w")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	while ((ch = getc(fp1)) != EOF) {
		putc(ch, fp3);
	}
	while ((ch = getc(fp2)) != EOF) {
		putc(ch, fp3);
	}

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	exit(0);
}
