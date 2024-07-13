#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define NAME_SIZE 256

int main(int argc, char *argv[]) {
	FILE *fp1, *fp2;
	char fname1[NAME_SIZE], fname2[NAME_SIZE];
	char buf1[NAME_SIZE], buf2[NAME_SIZE];

	printf("Enter your first file name: ");
	scanf("%s", buf1);
	printf("Enter your second file name: ");
	scanf("%s", buf2);

	if ((fp1 = fopen(buf1, "r")) == NULL || (fp2 = fopen(buf2, "r")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	int ok = 0;
	do {
		*buf1 = *buf2 = 0;
		fgets(buf1, NAME_SIZE, fp1);
		fgets(buf2, NAME_SIZE, fp2);
		if (strcmp(buf1, buf2)) ok = 1;
	} while (*buf1 || *buf2);

	fclose(fp1);
	fclose(fp2);
	
	printf("%s\n", ok ? "Given two files are not identical" : "Given two file have identical contents");

	exit(0);
}
