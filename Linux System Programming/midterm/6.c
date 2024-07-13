#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define NAME_SIZE 256

int main(int argc, char *argv[]) {
	FILE *fp1, *fp2, *fp3;
	char buf1[NAME_SIZE], buf2[NAME_SIZE], buf3[NAME_SIZE] = {0,};
	char fname1[NAME_SIZE], fname2[NAME_SIZE], temp[] = "temp.txt";

	strcpy(fname1, argv[1]);
	strcpy(fname2, argv[2]);

	if ((fp1 = fopen(fname1, "r")) == NULL || (fp2 = fopen(fname2, "r")) == NULL || (fp3 = fopen(temp, "w")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	for (int i = 0; !feof(fp1) || !feof(fp2); i++) {
		if (i % 2 == 0 || feof(fp2)) {
			fgets(buf1, NAME_SIZE, fp1);
			fprintf(fp3, "%s", buf1);
		//	strcat(buf3, buf1);
			//strcat(buf3, "\n");
		} else {
			fgets(buf2, NAME_SIZE, fp2);
			fprintf(fp3, "%s", buf2);
			//strcat(buf3, buf2);
			//strcat(buf3, "\n");
		}
	}

	fprintf(fp3, "%s", buf3);
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	remove(fname1);
	remove(fname2);
	rename(temp, fname1);

	exit(0);
}
