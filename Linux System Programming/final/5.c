#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

FILE *old_fp;
FILE *new_fp;

void search_files(char *dir_path, time_t compare_time) {
	struct dirent* entry;
	struct stat file_stat;
	DIR *dir;

	stat(dir_path, &file_stat);
	if (S_ISREG(file_stat.st_mode)) {
		fprintf(file_stat.st_mtime <= compare_time ? old_fp : new_fp, "%30s %30ld %30ld %30ld %30ld\n", dir_path, file_stat.st_mtime, file_stat.st_ctime, file_stat.st_atime, file_stat.st_size);
		return;
	}

	dir = opendir(dir_path);
	while ((entry = readdir(dir))) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
		char next[4096];
		sprintf(next, "%s/%s", dir_path, entry->d_name);
		search_files(next, compare_time);
	}

	closedir(dir);
}

int main(int argc, char *argv[]) {
	char *old_fname = "old.file";
	char *new_fname = "new.file";
	char *dir_path = argv[1];
	time_t compare_time = atoi(argv[2]);

	if (argc != 3) {
		printf("Usage : %s <directory path> <compare time>\n", argv[0]);
		return 1;
	}

	if ((old_fp = fopen(old_fname, "w")) == NULL || (new_fp = fopen(new_fname, "w")) == NULL) {
		fprintf(stderr, "fopen error\n");
		exit(1);
	}

	fprintf(old_fp, "%30s %30s %30s %30s %30s\n", "Path", "Modified Time", "Changed Time", "Accessed Time", "File Size");
	fprintf(new_fp, "%30s %30s %30s %30s %30s\n", "Path", "Modified Time", "Changed Time", "Accessed Time", "File Size");

	search_files(dir_path, compare_time);

	fclose(old_fp);
	fclose(new_fp);

	exit(0);
}
