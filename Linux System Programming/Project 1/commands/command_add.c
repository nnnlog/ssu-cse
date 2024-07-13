#include <string.h>
#include <stdio.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "init.h"
#include "../usage/usage.h"
#include "../file/file.h"
#include "../debug/debug.h"
#include "../file/hash.h"
#include "../file/path.h"

int command_add_execute(char *input_path, int is_recursive);

void command_add_backup(const char *path);

// argv : <path> <0|1> / <filename> [option]
int main(int argc, char *argv[]) {
    if (argc < 3) { // 위 arguments 형태를 보면 알 수 있듯이 arguments는 최소 3개 들어와야 함
        print_help_add();
        return 0;
    }

    init(argc, argv); // 들어온 argument 로 각종 경로 작업

    int filename_argument_exist = 1;
    if (argv[2][0] == '-') { // filename이 와야 할 위치에 -가 오면 무조건 명령어 옵션 취급, 디렉토리나 파일이 -로 시작하려면 ./-blabla 처럼 입력해야 함.
        filename_argument_exist = 0;
    }

    int is_recursive = 0;
    optind = 3;

    int option;
    while ((option = getopt(argc, argv, "d")) != -1) {
        switch (option) {
            case 'd':
                is_recursive = 1;
                break;
            default:
                print_help_add();
                return 0;
        }
    }

    if (argc != optind) { // 인자가 끝까지 파싱되지 않았으면 invalid한 인자가 있었다는 뜻임
        print_help_add();
        return 0;
    }

    if (!command_add_execute(filename_argument_exist ? argv[2] : NULL, is_recursive)) { // 리턴값이 0이면, 잘못된 인자가 있었다는 뜻. usage 호출함
        print_help_add();
    }
}

int command_add_execute(char *input_path, int is_recursive) {
    if (!input_path) { // <filename>이 안 주어질 때
        debug_print("filename not given\n");
        return 0;
    }

    char path[PATH_MAX * 2];
    get_absolute_path(input_path, path); // 절대 경로로 변환
    debug_print("REAL PATH : '%s'\n", path);

    if (strlen(path) > PATH_MAX) {
        printf("file argument length exceed, given size : %ld\n", strlen(path));
        return 1;
    }

    if (!check_path_in_directory(path, USER_DIR)) { // path가 홈 디렉터리를 벗어나면
        debug_print("out of home folder\n");
        printf("\"%s\" can't be backuped\n", input_path);
        return 1;
    }

    if (access(path, R_OK) < 0) { // 백업할 땐 read만 되면 됨
        printf("cannot access file \"%s\" : %s\n", input_path, strerror(errno));
        return 0;
    }

    {
        int is_dir = is_directory(path);
        if (is_dir < 0) {
            printf("\"%s\" is not regular path and directory\n", input_path);
            return 0;
        }
        if (is_dir && !is_recursive) {
            printf("\"%s\" is a directory file, need -d option\n", input_path);
            return 1;
        }
    }

    // 입력 받은 경로가 백업 디렉토리에 접근하는 경우는 2가지 중 하나
    // case 1) 유저 디렉터리와 동일한 경로
    // case 2) 백업 디렉토리 내 경로 (백업 디렉토리와 동일한 경우 포함)
    if (!strcmp(USER_DIR, path) || check_path_in_directory(path, BACKUP_DIR)) {
        printf("\"%s\" can't be backuped\n", input_path);
        return 1;
    }

    command_add_backup(path); // 유효성 검사 끝, 백업 시작

    return 1;
}

void command_add_backup(const char *path) {
    char date[20];
    {
        time_t tm = time(NULL);
        struct tm *tm_ptr = localtime(&tm);
        sprintf(date, "%02d%02d%02d%02d%02d%02d", (tm_ptr->tm_year + 1900) % 100, (tm_ptr->tm_mon + 1), tm_ptr->tm_mday,
                tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec); // date 포맷은 미리 만들어두고 계속 사용함
    }

    debug_print("date : '%s'\n", date);

    struct Node *root = find_all_files(path, 1, 1); // path 아래의 모든 파일 가져옴 (또는 path를 경로로 가지는 regular file)
    struct Node *node = root;

    while (node) {
        debug_print("FILE : %s\n", node->path);
        debug_print("relav : %s\n", get_relative_path(node->path, USER_DIR));

        int skip_file = 0;
        {
            char backup_dir[PATH_MAX];
            sprintf(backup_dir, "%s/%s", BACKUP_DIR, get_relative_path(node->path, USER_DIR));

            struct Node *_root = find_same_name_files(backup_dir); // 백업 디렉토리 내 동일한 파일 목록 가져옴
            struct Node *_node = _root;
            while (_node) {
                if (compare_file(node->path, _node->path)) { // 동일한 파일이면
                    printf("\"%s\" is already backuped\n", _node->path);
                    skip_file = 1; // 백업 안하고 넘어감
                    break;
                }
                _node = _node->next;
            }
            clear_node(_root);
        }

        if (!skip_file) {
            char backup_path[PATH_MAX];
            sprintf(backup_path, "%s/%s_%s", BACKUP_DIR, get_relative_path(node->path, USER_DIR), date);

            copy_file(backup_path, node->path); // 백업함
            printf("\"%s\" backuped\n", backup_path);
        }

        node = node->next;
    }
    clear_node(root);
}
