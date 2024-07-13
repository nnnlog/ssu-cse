#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define BUFFER_SIZE 1024
#define PATHMAX 4096

typedef struct Dir {
	struct Dir* next;
	char src[PATHMAX], dst[PATHMAX];
} Dir;

Dir *create_node(char *src, char *dst) {
	Dir *node = malloc(sizeof (struct Dir));
	node->next = NULL;
	strcpy(node->src, src);
	strcpy(node->dst, dst);
	return node;
}

char *get_file_name(char *path) {
	char *last = path;
	for (char *i = path; *i; i++) if (*i == '/') last = i;
	return last;
}

char *get_file_path(char *path, char *name) {
	char *ret = malloc(strlen(path) + strlen(name) + 2);
	sprintf(ret, "%s/%s", path, name);
	return ret;
}

int is_dir(const char *path) {
	struct stat buf;
	if (lstat(path, &buf) < 0) return -1;
	return S_ISDIR(buf.st_mode);
}

int is_reg(const char *path) {
	struct stat buf;
	if (lstat(path, &buf) < 0) return -1;
	return S_ISREG(buf.st_mode);
}

void copy(char *src, char *dst) {
	for (char *i = dst + strlen(dst) - 1; i != dst; i--) {
		if (*i == '/') {
			*i = 0;
			mkdir(dst, 0777);
			*i = '/';
		}
	}

	FILE *f1, *f2;
	if (!(f1 = fopen(src, "r")) || !(f2 = fopen(dst, "w"))) {
		printf("%s %s\n", src, dst);
		fprintf(stderr, "fopen error\n");
		exit(1);
	}
	char buf[PATHMAX];
	while (fgets(buf, PATHMAX, f1)) fputs(buf, f2);
	fclose(f1);
	fclose(f2);
}

Dir *root;
void do_add(char *src, char *dst) {
	if (!is_dir(src) && !is_reg(src)) {
		fprintf(stderr, "not regular file or directory\n");
		exit(1);
	}
	if (is_reg(src)) {
		struct Dir *curr = create_node(src, dst);
		if (!root) root = curr;
		else {
			struct Dir *r = root;
			while (r->next) r = r->next;
			r->next = curr;
		}
		if (!strcmp(src, dst)) {
			fprintf(stderr, "src and backup file equals\n");
			exit(1);
		}
//		copy(src, dst);
//		printf("(%s) backuped to (%s)\n", src, dst);
		return;
	} else {
		DIR *dir = opendir(src);
		if (!dir) {
			fprintf(stderr, "opendir error for %s\n", src);
			exit(1);
		}
		struct dirent *dp;
		while (dp = readdir(dir)) {
			if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) continue;
			char *next1 = get_file_path(src, dp->d_name);
			char *next2 = get_file_path(dst, dp->d_name);
			do_add(next1, next2);
		}
		closedir(dir);
	}
}

void command_add(char *srci, char *dsti) {
	char src[PATHMAX], dst[PATHMAX];
	realpath(srci, src);
	realpath(dsti, dst);

	int d = is_dir(dst);
	if (d == 0) {
		fprintf(stderr, "%s is not directory\n", dst);
		exit(1);
	} else if (d == -1) {
		mkdir(dst, 0777);
	}
	d = is_dir(dst);
	if (d != 1) {
		fprintf(stderr, "failed to make directory\n");
		exit(1);
	}
	if (!is_reg(src) && !is_dir(src)) {
		fprintf(stderr, "not regular file or directory\n");
		exit(1);
	}

	if (!strcmp(src, dst)) {
		fprintf(stderr, "src and dst equals\n");
		exit(1);
	}

	if (access(src, R_OK) || access(dst, W_OK)) {
		fprintf(stderr, "no permission\n");
		exit(1);
	}

	strcat(src, "/");
	strcat(dst, "/");

	if (strstr(src, dst) == src || strstr(dst, src) == dst) {
		fprintf(stderr, "recursive detect\n");
		exit(1);
	}
	
	src[strlen(src) - 1] = 0;
	dst[strlen(dst) - 1] = 0;

	do_add(src, dst);

	if (root == NULL) {
		fprintf(stderr, "empty file\n");
		exit(1);
	}

	// sort!!
	{
		int length = 0;
		struct Dir *node = root;
		while (node) ++length, node = node->next;
		node = root;
		struct Dir **arr = malloc(length * sizeof(struct Dir*));
		for (int i = 0; i < length; i++) {
			arr[i] = node;
			node = node->next;
		}

		for (int i = 0; i < length; i++) {
			for (int j = 0; j < length; j++) {
				// compare condition (only by src) : 1. depth / 2. alphabet
				int ca = 0, cb = 0;
				for (char *k = arr[i]->src; *k; k++) ca += *k == '/';
				for (char *k = arr[j]->src; *k; k++) cb += *k == '/';
				int swap = 0;
				if (ca < cb) swap = 1;
				else if (strcmp(arr[i]->src, arr[i]->src) > 0) swap = 1;
				
				if (swap) {
					struct Dir *tmp = arr[i];
					arr[i] = arr[j];
					arr[j] = tmp;
				}
			}
		}

		root = arr[0];
		node = root;
		for (int i = 1; i < length; i++) {
			node->next = arr[i];
			node = node->next;
		}
		node->next = 0;
	}

	struct Dir *curr = root;
	while (curr) {
		copy(curr->src, curr->dst);	
		printf("(%s) backuped to (%s)\n", curr->src, curr->dst);
		curr = curr->next;
	}
}

int main(int argc, char *argv[]) {
//	printf("%s\n", normalize_path("../..///asdf/dasf123"));
	
	if (argc < 3) {
		fprintf(stderr, "usage : %s <OLD_PATH> <BACKUP_DIR>\n", argv[0]);
		exit(1);
	}

	command_add(argv[1], argv[2]);

	exit(0);
}
