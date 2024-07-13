#include "file.h"
#include "path.h"
#include "../data/linked_list.h"
#include "../debug/debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

// free return value
// regular file 경로를 Node value로 하는 링크드 리스트를 리턴함
struct Node *find_all_files(const char *path, int is_recursive, int is_first) {
    int is_dir = is_directory(path);
    if (is_dir < 0) { // regular file/directory가 둘 다 아님
        return NULL;
    }
    if (!is_dir) { // regular file
        return create_node(path);
    }
    if (!is_recursive && !is_first) return NULL; // 첫번째 실행이 아니면서, 재귀적으로 탐색하지 않는다면 탑색 중지
    struct Node *node = NULL;
    struct dirent **list;
    int cnt = scandir(path, &list, NULL, NULL);
    for (int i = 0; i < cnt; i++) {
        char *name = list[i]->d_name;
        if (!strcmp(name, ".") || !strcmp(name, "..")) continue;

        char next[PATH_MAX];
        sprintf(next, "%s/%s", path, name);

        struct Node *tmp = find_all_files(next, is_recursive, 0); // 다음 함수 호출
        if (tmp != NULL) {
            if (node == NULL) node = tmp;
            else get_last_node(node)->next = tmp; // here is O(N)
        }
    }
    if (cnt >= 0) {
        for (int i = 0; i < cnt; i++) free(list[i]);
        free(list);
    } else
            debug_print("scandir error (%s) : %s\n", path, strerror(errno));
    return node;
}

// free return value
// path와 같은 경로에 있는 파일 중, path와 파일 이름(<date>를 제외한 경로)이 동일한 regular file의 경로를 Node value로 하는 링크드 리스트를 리턴함
struct Node *find_same_name_files(const char *path) {
    char absolute_path[PATH_MAX];
    get_absolute_path(path, absolute_path);

    char dir[PATH_MAX];
    get_parent_directory(path, dir);
    debug_print("SCAN DIR : '%s'\n", dir);

    struct Node *ret_root = NULL;

    struct Node *root = find_all_files(dir, 0, 1); // find_all_files로 같은 폴더 내에 있는 regular file을 모두 가져옴
    struct Node *node = root;
    while (node) {
        char plain_path[PATH_MAX];
        remove_filename_date(node->path, plain_path); // regular file path에서 _<DATE>는 제거함

        if (!strcmp(absolute_path, plain_path)) { // 나머지 경로가 같으면 링크드 리스트에 추가
            struct Node *next = create_node(node->path);
            if (ret_root == NULL) ret_root = next;
            else get_last_node(ret_root)->next = next; // here is O(N)
        }

        node = node->next;
    }
    clear_node(root);

    return ret_root;
}

// free return value
unsigned char *read_file(const char *path, int *length) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) return NULL;

    *length = get_filesize(path);

    unsigned char *ret = malloc(*length);
    memset(ret, 0, *length);

    for (int i = 0; i < *length; i++) ret[i] = getc(fp);

    fclose(fp);

    return ret;
}

void write_file(const char *path, const unsigned char *content, int length) {
    {
        char tmp[PATH_MAX];
        for (int i = 0; path[i]; i++) {
            tmp[i] = path[i];
            if (i && tmp[i] == '/') { // "/" 가 나올 때마다, mkdir를 호출해서 디렉토리를 만들어줌
                tmp[i] = 0;

                if (is_directory(tmp) == -1) { // 디렉터리가 존재하지 않는다면
                    if (mkdir(tmp, 0777) < 0) {
                        debug_print("mkdir failed (%s) : %s\n", tmp, strerror(errno));
                        return;
                    }
                }

                tmp[i] = '/';
            }
        }
    }

    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        debug_print("fopen failed (%s) : %s\n", path, strerror(errno));
    } else {
        fwrite(content, sizeof(char), length, fp);
        fclose(fp);
    }
}

void copy_file(const char *dst, const char *src) {
    int length;
    unsigned char *content = read_file(src, &length);
    write_file(dst, content, length);
    free(content);
}

off_t get_filesize(const char *path) {
    struct stat info;
    if (lstat(path, &info) < 0) return -1;
    return info.st_size;
}

int remove_backup_directory_safely(const char *path) {
    int ret = 1;

    struct dirent **list;
    int cnt = scandir(path, &list, NULL, NULL);
    if (cnt < 0) return 0; // already deleted or error

    int exist_file = 0;
    for (int i = 0; i < cnt; i++) {
        if (!strcmp(list[i]->d_name, ".") || !strcmp(list[i]->d_name, "..")) continue;

        if (list[i]->d_type != DT_DIR) { // 파일이 존재하면 작업을 취소함
            debug_print("exist file : %s (%d)\n", list[i]->d_name, list[i]->d_type);
            exist_file = 1;
            break;
        }
    }

    if (!exist_file) {
        for (int i = 0; i < cnt; i++) {
            if (!strcmp(list[i]->d_name, ".") || !strcmp(list[i]->d_name, "..")) continue;

            char next[PATH_MAX];
            sprintf(next, "%s/%s", path, list[i]->d_name);

            int t = remove_backup_directory_safely(next); // 하위 디렉토리 삭제를 요청함
            if (t < 0) ret = -1;
            else ret += t;
        }

        if (strcmp(path, BACKUP_DIR)) { // 현재 디렉토리가 백업 디렉토리가 아니라면, 현재 디렉토리를 삭제함
            if (rmdir(path) < 0) {
                ret = -1;
                debug_print("Fail to delete '%s' : %s\n", path, strerror(errno));
            } else debug_print("delete '%s'\n", path);
        } else ret--;
    }

    for (int i = 0; i < cnt; i++) free(list[i]);
    free(list);

    return ret;
}
