#include "daemon_tree.h"

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#include "../commons/const.h"

void log_file(FILE *fp, time_t tm, const char *head, const char *body) {
    time_t t;
    if (tm == 0) time(&t), tm = t;

    char buf[BUFFER_MAX];
    strftime(buf, BUFFER_MAX, "[%F %T]", localtime(&tm));
    fputs(buf, fp); // date

    if (head) fprintf(fp, "[%s]", head);
    if (body) fprintf(fp, "[%s]", body);
    fputs("\n", fp);
}

int cmp_by_filename_asc(const void *p1, const void *p2) {
    struct DaemonFileNode **a = (struct DaemonFileNode **) p1, **b = (struct DaemonFileNode **) p2; // 이중 포인터임
    return strcmp((*a)->path, (*b)->path);
}

struct DaemonFileNode *construct_tree(const char *path) {
    struct stat statbuf;
    if (lstat(path, &statbuf) < 0) return NULL;

    struct DaemonFileNode *node = malloc(sizeof(struct DaemonFileNode));

    node->list_cnt = 0;
    node->list = NULL;
    node->mtime = statbuf.st_mtim.tv_sec;

    node->path = malloc(strlen(path) + 1);
    strcpy(node->path, path);

    if (!S_ISDIR(statbuf.st_mode)) {
        return node;
    }

    DIR *dp;
    if ((dp = opendir(path)) == NULL) return NULL;

    struct dirent *dir;
    while ((dir = readdir(dp)) != NULL) {
        if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) continue;
        node->list_cnt++;
        if (node->list == NULL) node->list = malloc(node->list_cnt * sizeof(struct DaemonFileNode *));
        node->list = realloc(node->list, node->list_cnt * sizeof(struct DaemonFileNode *));

        char next[PATH_MAX];
        sprintf(next, "%s/%s", path, dir->d_name); // 경로 생성
        node->list[node->list_cnt - 1] = construct_tree(next); // 하나씩 Tree 만들어서 list에 넣음
    }

    qsort(node->list, node->list_cnt, sizeof(struct DaemonFileNode *), cmp_by_filename_asc); // 파일 이름 기준으로 list를 sorting 해줌

    closedir(dp);

    return node;
}

void free_tree(struct DaemonFileNode *node) {
    free(node->path);

    if (!node->list_cnt) {
        free(node);
        return;
    }

    for (int i = 0; i < node->list_cnt; i++) {
        free_tree(node->list[i]); // free도 재귀적으로 해줘야 함
    }

    free(node->list);
    free(node);
}

void compare_tree(struct DaemonFileNode *prev, struct DaemonFileNode *next, FILE *fp, const char *logPath) {
    if (!next && !prev->list_cnt) {
        log_file(fp, 0, "remove", prev->path);
    }

    if (!prev && !next->list_cnt) {
        log_file(fp, 0, "create", next->path);
    }

    // 로그 파일이면 무시함
    if ((prev && !strcmp(logPath, prev->path)) || next && !strcmp(logPath, next->path)) return;

    // traverse sub file using two-pointer (list is sorted by dictionary order)
    int l = 0, r = 0;
    if (prev && next) {
        if (!prev->list_cnt && !next->list_cnt && prev->mtime != next->mtime) { // mtime이 수정되었는지
            log_file(fp, next->mtime, "modify", prev->path);
        }

        while (l < prev->list_cnt && r < next->list_cnt) {
            int curr = strcmp(prev->list[l]->path, next->list[r]->path);
            if (!curr) {
                compare_tree(prev->list[l], next->list[r], fp, logPath); // 같은 경로라면 재귀 호출
                l++, r++;
            } else if (curr < 0) {
                compare_tree(prev->list[l], NULL, fp, logPath); // l을 한칸 뒤로 옮김, 이 경우엔 file delete가 일어난 것
                l++;
            } else {
                compare_tree(NULL, next->list[r], fp, logPath); // r을 한 칸 뒤로 옮김, 이 경우엔 file creation이 일어남
                r++;
            }
        }
    }

    // 남은거 마저 처리해줌
    for (; l < (prev ? prev->list_cnt : 0); l++) compare_tree(prev->list[l], NULL, fp, logPath);
    for (; r < (next ? next->list_cnt : 0); r++) compare_tree(NULL, next->list[r], fp, logPath);
}
