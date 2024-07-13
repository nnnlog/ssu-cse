#include "tree.h"

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

struct TreeCommandNode *root;

const char *get_filename(const char *path) {
    const char *ret = path;
    for (const char *i = path; *i; i++) {
        if (*i == '/') ret = i + 1;
    }
    return ret; // 파일 이름 리턴
}

int cmp_by_filename_asc(const void *p1, const void *p2) {
    struct dirent *a = (struct dirent *) p1, *b = (struct dirent *) p2;
    return strcmp(a->d_name, b->d_name);
}

void traverse(const char *path) {
    struct stat statbuf;
    if (lstat(path, &statbuf) < 0) return;

    struct TreeCommandNode *lastNode = root;
    if (root != NULL) {
        while (lastNode->next) {
            if (lastNode->remaining) printf("│   "); // 해당 ancestor의 자식이 더 남은 상태라면 | 출력
            else printf("    "); // 아니면 선을 그리지 않음
            lastNode = lastNode->next;
        }
        if (lastNode->remaining) printf("├"); // 부모의 자식이 더 남은 상태라면 아래 쪽으로 선을 그려줌
        else printf("└"); // 아니면 여기서 마감
        printf("── ");
    }
    printf("%s", get_filename(path));

    if (S_ISLNK(statbuf.st_mode)) {
        char buf[PATH_MAX];
        ssize_t sz = readlink(path, buf, PATH_MAX);
        buf[sz] = 0;
        printf(" -> %s", buf); // symbolic link
    }

    printf("\n");

    if (!S_ISDIR(statbuf.st_mode)) return;

    // 링크드 리스트 형태임 (다만, 현재 함수가 종료됨에 따라 다시 pop 해주기 때문에 동적 할당 안 해도 됨)
    struct TreeCommandNode node = {1, 0};

    if (root == NULL) root = &node;
    else lastNode->next = &node;


    DIR *dp;
    if ((dp = opendir(path)) == NULL) return;

    int cnt = 0;
    struct dirent *arr = NULL;

    struct dirent *dir;
    while ((dir = readdir(dp)) != NULL) {
        if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) continue;
        cnt++;
        if (arr == NULL) arr = malloc(sizeof(struct dirent) * cnt);
        else arr = realloc(arr, sizeof(struct dirent) * cnt);
        memcpy(&arr[cnt - 1], dir, sizeof(struct dirent));
    }

    qsort(arr, cnt, sizeof(struct dirent), cmp_by_filename_asc);

    for (int i = 0; i < cnt; i++) {
        if (i + 1 == cnt) node.remaining = 0; // 마지막 노드이면 remaining을 0으로 변경해줌

        char next[PATH_MAX];
        sprintf(next, "%s/%s", path, arr[i].d_name);
        traverse(next); // 계속해서 traverse 호출
    }

    // 마지막 노드 빼기
    if (lastNode == NULL) root = NULL;
    else lastNode->next = NULL;

    free(arr);
    closedir(dp);
}

void print_as_tree(const char *path) {
    root = NULL;
    traverse(path);
}
