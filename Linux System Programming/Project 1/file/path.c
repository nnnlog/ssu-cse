#include "path.h"
#include "../debug/debug.h"

#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>


char BACKUP_DIR[PATH_MAX];
char USER_DIR[PATH_MAX];

char EXECUTE_DIR[PATH_MAX];

void init_filesystem(char *executable_path) {
    get_absolute_path(getenv("HOME"), USER_DIR);
//    get_absolute_path("/home/nlog", USER_DIR);
    debug_print("Set user directory to %s\n", USER_DIR);

    sprintf(BACKUP_DIR, "%s/%s", USER_DIR, "backup");
    debug_print("Set backup directory to %s\n", BACKUP_DIR);

    if (is_directory(BACKUP_DIR) == -1) mkdir(BACKUP_DIR, 0777); // 백업 디렉토리 없으면 생성
    if (is_directory(BACKUP_DIR) != 1 || access(BACKUP_DIR, R_OK | W_OK) < 0) { // 그래도 없거나, 접근 권한이 부족하면 프로그램 종료
        printf("Failed to create backup directory or not have permission (%s)\n", BACKUP_DIR);
        exit(0);
    }

    // 실행 파일의 위치를 찾는다.
    // 기본적으로 ssu_backup과 command_*의 실행 파일 위치는 동일하다고 가정한다.
    get_absolute_path(executable_path, EXECUTE_DIR);
    if (access(EXECUTE_DIR, X_OK) < 0) { // 상대경로(혹은 절대경로)에서 프로그램 파일을 찾지 못한다면 $PATH에서 검색을 시도한다.
        char var_path[PATH_MAX];
        strcpy(var_path, getenv("PATH"));

        int ok = 0;
        char *dir = var_path;
        while ((dir = strtok(dir, ":")) != NULL) {
            char tmp[PATH_MAX];
            sprintf(tmp, "%s/%s", dir, executable_path);
            get_absolute_path(tmp, tmp);

            if (!access(tmp, X_OK)) {
                ok = 1;
                strcpy(EXECUTE_DIR, tmp);
                break;
            }

            dir = dir + strlen(dir) + 1;
        }

        if (!ok) {
            printf("[ERROR] cannot find executable path.\n");
        }
    }

    get_parent_directory(EXECUTE_DIR, EXECUTE_DIR); // 실행 파일이 있는 위치의 디렉터리를 EXECUTE_DIR로 정의
    debug_print("Executable binary directory : %s\n", EXECUTE_DIR);
}

/**
 * path를 받으면 절대 경로로 변환한다. (또는, 경로 정규화[normalize path]라고 한다.)
 * realpath 함수는 매개변수 path를 절대경로로 변환한 후, 파일이나 디렉터리가 없으면 realpath에서 계속 경로를 삭제하거나 ENONET를 발생시켜서 올바른 절대 경로를 가져오기가 어려움
 */
void get_absolute_path(const char *path, char *ret) {
    if (!path) return;
    char *input = malloc(strlen(path) + PATH_MAX + 2);
    if (*path == '/') { // 절대 경로인가?
        strcpy(input, path);
    } else if (*path == '~') { // ~ 확장이 필요한가?
        strcpy(input, USER_DIR);
        strcat(input, "/");
        strcat(input, path + 1);
    } else { // 상대 경로이므로 pwd를 앞에 붙여준다.
        getcwd(input, strlen(path) + PATH_MAX + 2);
        strcat(input, "/");
        strcat(input, path);
    }

    strcat(input, "/");

    // 고려할 것
    // case 1) /./
    // case 1-1) //
    // case 2) /../
    // case 3) root dir에서 ../ 시도

    // 절대 경로를 구하는 동안, res의 끝에는 "/"를 붙이지 않는다.
    // 루트 경로(/)일 때만(문자열이 empty인 경우) 리턴할 때 붙여준다.

    char buffer[PATH_MAX] = {0};
    char *ptr = buffer;

    char *res_ptr = ret;

    for (char *i = input; *i; i++) {
        if (*i == '/') {
            *ptr = 0;
            if (!strcmp(buffer, ".") || !*buffer) { // "//" 거나 "/./" 인 경우, current directory
                // do not anything
            } else if (!strcmp(buffer, "..")) { // parent directory
                while (ret < res_ptr && *--res_ptr != '/');
            } else { // 단순 cd
                *res_ptr++ = '/';
                for (char *j = buffer; *j; j++) *res_ptr++ = *j;
            }
            ptr = buffer;
        } else {
            *ptr++ = *i;
        }
    }

    if (res_ptr == ret) *res_ptr++ = '/';
    *res_ptr = 0;

    free(input);
}

const char *get_relative_path(const char *path, const char *base) {
    if (!check_path_in_directory(path, base)) return NULL;
    return path + strlen(base) + 1;
}

const char *get_date_in_path(const char *path) {
    for (int i = strlen(path) - 1; i >= 0; i--) {
        if (path[i] == '_') return path + i + 1;
    }
    return NULL;
}

void get_parent_directory(const char *path, char *ret) {
//    char *ret = malloc(strlen(path) + 1);
    strcpy(ret, path);
    {
        int i = strlen(ret);
        for (; ret[i] != '/'; i--);
        ret[i] = 0;
    }
}

// path가 /home/nlog/backup_1234이고, backup directory가 /home/nlog/backup이라고 가정하자.
// 이 경우에 단순하게 strstr(path, backup directory)를 사용하면 실제로는 backup_directory에 속하지 않는 경로들이 걸러진다.
// 따라서, 경로 정규화(normalize path)를 실행하고, "/"를 붙인 후 strstr을 사용하여 비교를 진행한다.
int check_path_in_directory(const char *path, const char *directory) {
    char path_tmp[PATH_MAX];
    get_absolute_path(path, path_tmp);
    strcat(path_tmp, "/"); // 파일일 때 붙여도 전혀 상관 없음

    char dir_tmp[PATH_MAX];
    get_absolute_path(directory, dir_tmp);
    strcat(dir_tmp, "/");

    return strstr(path_tmp, dir_tmp) == path_tmp;
}

void remove_filename_date(const char *path, char *ret) {
    int last_idx = strlen(path);
    for (int i = 0; path[i]; i++) {
        if (path[i] == '_') {
            last_idx = i;
        }
    }

//    char *ret = malloc(last_idx + 1);
    for (int i = 0; i < last_idx; i++) ret[i] = path[i];
    ret[last_idx] = 0;
}

// -1 : fail (not file and directory), 0 : file, 1 : directory
int is_directory(const char *path) {
    struct stat file_stat;
    if (lstat(path, &file_stat) < 0) {
        return -1;
    }
    return S_ISDIR(file_stat.st_mode) ? 1 : (S_ISREG(file_stat.st_mode) ? 0 : -1);
}
