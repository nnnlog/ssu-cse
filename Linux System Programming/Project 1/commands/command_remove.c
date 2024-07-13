#include <string.h>
#include <stdio.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>

#include "init.h"
#include "../usage/usage.h"
#include "../file/file.h"
#include "../file/path.h"
#include "../debug/debug.h"
#include "../file/hash.h"

int command_remove_execute(char *input_path, int is_recursive, int is_clear);

void command_remove_backup(const char *path, int is_recursive, int is_clear);

// argv : <path> <0|1> / [filename] [option]
int main(int argc, char *argv[]) {
    if (argc < 3) { // command_remove 0 filename = 3, command_remove 0 -c = 3 (minimum size: 3)
        print_help_remove();
        return 0;
    }

    init(argc, argv); // 들어온 argument 로 각종 경로 작업

    int filename_argument_exist = 1;
    if (argv[2][0] == '-') { // filename이 와야 할 위치에 -가 오면 무조건 명령어 옵션 취급, 디렉토리나 파일이 -로 시작하려면 ./-blabla 처럼 입력해야 함.
        filename_argument_exist = 0;
    }

    int is_clear = 0;
    int is_recursive = 0;
    optind = filename_argument_exist ? 3 : 2;

    int option;
    while ((option = getopt(argc, argv, "ac")) != -1) {
        switch (option) {
            case 'a':
                is_recursive = 1;
                break;
            case 'c':
                is_clear = 1;
                break;
            default:
                print_help_remove();
                return 0;
        }
    }

    if (argc != optind) { // 인자가 끝까지 파싱되지 않았으면 invalid한 인자가 있었다는 뜻임
        print_help_add();
        return 0;
    }

    if (!command_remove_execute(filename_argument_exist ? argv[2] : NULL, is_recursive, is_clear)) { // 리턴값이 0이면, 잘못된 인자가 있었다는 뜻. usage 호출함
        print_help_remove();
    }
}

int command_remove_execute(char *input_path, int is_recursive, int is_clear) {
    if (is_recursive && is_clear) { // clear 옵션이 사용됐을 때, -a 옵션이 주어지면
        printf("option 'a' and 'c' duplicated\n");
        return 0;
    }

    if (input_path && is_clear) { // clear 옵션이 사용됐을 때, 파일 경로가 주어지면
        printf("option 'c' and argument 'file path' duplicated\n");
        return 0;
    }

    if (is_clear) { // clear 옵션이 사용되면 전부 삭제
        command_remove_backup(BACKUP_DIR, 1, 1);
        return 1;
    }

    if (!input_path) { // <filename>이 안 주어질 때
        debug_print("filename not given\n");
        return 0;
    }

    char path[PATH_MAX * 2];
    get_absolute_path(input_path, path);

    if (strlen(path) > PATH_MAX) {
        printf("file argument length exceed, given size : %ld\n", strlen(path));
        return 1;
    }

    if (!check_path_in_directory(path, USER_DIR)) { // path가 홈 디렉터리를 벗어나면
        printf("\"%s\" can't be removed.\n", input_path);
        return 1;
    }

    // 입력 받은 경로가 백업 디렉토리에 접근하는 경우는 2가지 중 하나
    // case 1) 유저 디렉터리와 동일한 경로
    // case 2) 백업 디렉토리 내 경로 (백업 디렉토리와 동일한 경우 포함)
    if (!strcmp(USER_DIR, path) || check_path_in_directory(path, BACKUP_DIR)) {
        printf("\"%s\" can't be removed.\n", input_path);
        return 1;
    }

    {
        char tmp[PATH_MAX];
        sprintf(tmp, "%s/%s", BACKUP_DIR, get_relative_path(path, USER_DIR));
        get_absolute_path(tmp, path);
    }

    command_remove_backup(path, is_recursive, 0);

    return 1;
}

void command_remove_backup(const char *path, int is_recursive, int is_clear) {
    int find_erase_file = 0;
    int deleted_file = 0;
    int deleted_dir = 0;

    debug_print("Delete path : %s\n", path);

    {
        struct Node *root = find_same_name_files(path); // path와 이름이 동일한 regular file만 모두 찾는다.
        int size = get_size_linked_list(root);

        debug_print("same file count : %d\n", size);

        if (size >= 1) {
            find_erase_file = 1;

            {
                struct Node *node = root;
                while (node) {
                    if (access(node->path, R_OK | W_OK) < 0) { // 권한 학인
                        printf("'%s' doesn't have read/write permission.\n", node->path);
                        clear_node(root);
                        exit(0);
                    }
                    if (is_directory(node->path) < 0) { // 일반 파일/디렉토리인지 확인
                        printf("'%s' is not regular file and directory.\n", node->path);
                        clear_node(root);
                        exit(0);
                    }
                    node = node->next;
                }
            }

            struct Node *node = root;
            if (size > 1 && !is_recursive) { // 개수가 1개 초과이면서 -a 옵션이 없으면, 프롬프트 제공해서 하나만 선택하도록 함
                int remove_index = 0;

                {
                    char tmp[PATH_MAX];
                    sprintf(tmp, "%s/%s", USER_DIR, get_relative_path(path, BACKUP_DIR));

                    printf("backup file list of \"%s\"\n", tmp);
                }

                printf("0. exit\n");
                for (int i = 0; i < size; i++) {
                    printf("%d. %s\t\t%'ldbytes\n", i + 1, get_date_in_path(node->path), get_filesize(node->path));
                    node = node->next;
                }
                printf("Choose file to remove\n>> ");

                scanf("%d", &remove_index);

                if (remove_index < 1 || remove_index > size) {
                    debug_print("Exit\n");
                    clear_node(root);
                    exit(0); // 0번 입력은 즉시 명령어 실행 종료
                } else {
                    node = root;
                    while (--remove_index) node = node->next;

                    if (!is_clear) printf("\"%s\" backup file removed\n", node->path);
                    unlink(node->path);
                    deleted_file++;
                }
            } else {
                // remove all
                deleted_file += size;
                while (size--) {
                    if (!is_clear) printf("\"%s\" backup file removed\n", node->path);
                    unlink(node->path);

                    node = node->next;
                }
            }
        }

        clear_node(root);
    }

    if (is_directory(path) == 1) {
        if (!is_recursive) {
            // 이미 regular file 하나를 삭제했으면, 동일한 이름의 디렉토리가 있으면 오류 처리 안 함
            if (!find_erase_file) {
                printf("\"%s\" is a directory file, need -a option\n", path);
            }
            return;
        }

        struct Node *root = find_all_files(path, 1, 1);
        struct Node *node = root;

        { // 파일 권한/타입 확인
            struct Node *node = root;
            while (node) {
                if (access(node->path, R_OK | W_OK) < 0) {
                    printf("%s doesn't have read/write permission.\n", node->path);
                    clear_node(root);
                    return;
                }
                if (is_directory(node->path) < 0) {
                    printf("%s is not file and directory.\n", node->path);
                    clear_node(root);
                    return;
                }
                node = node->next;
            }
        }

        debug_print("file count in directory : %d\n", get_size_linked_list(root));

        // remove all scanned file, 이때는 directory는 삭제 안하고 regular 파일만 삭제함
        while (node) {
            if (!is_clear) printf("\"%s\" backup file removed\n", node->path);
            unlink(node->path);
            deleted_file++;
            find_erase_file = 1;

            node = node->next;
        }

        clear_node(root);

        if (is_clear) { // clear인 경우에만 폴더 삭제
            int res = remove_backup_directory_safely(path);
            if (res < 0) printf("Fail to delete directory \"%s\"\n", path);
            else deleted_dir += res;
        }
    }

    if (is_clear) { // -c 옵션이 있었을 때,
        if (deleted_file || deleted_dir) // 삭제된 파일이 하나라도 있으면, (directory만 이라도 있으면 됨)
            printf("backup directory cleared(%d regular files and %d subdirectories totally).\n", deleted_file,
                   deleted_dir);
        else printf("no file(s) in the backup\n");
    } else if (!find_erase_file) {
        print_help_remove(); // 백업 파일이 없는 경우이므로 usage 출력 (굉장히 특수한 case임..)
        exit(0);
    }
}
