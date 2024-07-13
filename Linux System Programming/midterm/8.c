#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

FILE *old_fp;
FILE *new_fp;

void search_files(char* dir_path, time_t compare_time) {
	DIR *dir;

	struct stat statbuf;
	stat(dir_path, &statbuf);
	if (S_ISREG(statbuf.st_mode)) {
		FILE *d = new_fp;
		if (statbuf.st_ctime <= compare_time) d = old_fp;
		fprintf(d, "%-30s % -30ld % -30ld % -30ld % -30ld\n", dir_path, statbuf.st_mtime, statbuf.st_ctime, statbuf.st_atime, statbuf.st_size);
		return;
	}

	dir = opendir(dir_path);

	struct dirent *dp;
	while (dp = readdir(dir)) {
		if (!dp->d_ino || !strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) continue;
		char next[4096];
		sprintf(next, "%s/%s", dir_path, dp->d_name);
		search_files(next, compare_time);
	}

	closedir(dir);
}

int main(int argc, char *argv[]) {
	if (!(old_fp = fopen("old.file", "w")) || !(new_fp = fopen("new.file", "w"))) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}


	fprintf(old_fp, "%-30s %-30s %-30s %-30s %-30s\n", "Path", "Modified Time", "Changed Time", "Accessed Time", "File Size");
	fprintf(new_fp, "%-30s %-30s %-30s %-30s %-30s\n", "Path", "Modified Time", "Changed Time", "Accessed Time", "File Size");
	
	search_files(argv[1], atoi(argv[2]));

	fclose(old_fp);
	fclose(new_fp);

	exit(0);
}
