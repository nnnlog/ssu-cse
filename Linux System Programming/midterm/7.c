#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define NAME_SIZE 256

int main(int argc, char *argv[]) {
	FILE *fp1, *fp2, *fp3;
	char fname1[NAME_SIZE], fname2[NAME_SIZE];

	printf("Enter your first file name: ");
	scanf("%s", fname1);
	printf("Enter your second file name: ");
	scanf("%s", fname2);

	if ((fp1 = fopen(fname1, "r")) == NULL || (fp2 = fopen(fname2, "r")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	int f = 0;

	for (int i = 0; !feof(fp1) && !feof(fp2); i++) {
		char buf1[4096], buf2[4096];
		fgets(buf1, 4096, fp1);
		fgets(buf2, 4096, fp2);

		if (strcmp(buf1, buf2)) f = 1;
	}

	if (feof(fp1) != feof(fp2)) f = 1;

	printf("%s\n", f ? "Given two files are not identical" : "Given two file have identical contents");

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	exit(0);
}
