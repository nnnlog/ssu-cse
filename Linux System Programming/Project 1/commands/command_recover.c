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

int command_recover_execute(char *input_src, char *input_dest, int is_recursive);

void command_recover_process_file_list(const char *src_path, const char *dest_path, int is_recursive);

int command_recover_single_file(const char *backup_path, const char *dest_path);

// argv : <path> <0|1> / <filename> [option]
int main(int argc, char *argv[]) {
    if (argc < 3) { // command_recover 0 filename = 3 (minimum size: 3)
        print_help_recover();
        return 0;
    }

    init(argc, argv); // 들어온 argument 로 각종 경로 작업

    int filename_argument_exist = 1;
    if (argv[2][0] == '-') { // filename이 와야 할 위치에 -가 오면 무조건 명령어 옵션 취급, 디렉토리나 파일이 -로 시작하려면 ./-blabla 처럼 입력해야 함.
        filename_argument_exist = 0;
    }

    int is_recursive = 0;
    char input_dest[PATH_MAX] = {0,};
    optind = filename_argument_exist ? 3 : 2;

    int option;
    while ((option = getopt(argc, argv, ":dn:")) != -1) {
        switch (option) {
            case 'd':
                is_recursive = 1;
                break;
            case 'n':
                strcpy(input_dest, optarg);
                break;
            default:
                print_help_recover();
                return 0;
        }
    }

    if (argc != optind) { // 인자가 끝까지 파싱되지 않았으면 invalid한 인자가 있었다는 뜻임
        print_help_add();
        return 0;
    }

    if (!command_recover_execute(filename_argument_exist ? argv[2] : NULL, *input_dest ? input_dest : NULL,
                                 is_recursive)) { // 리턴값이 0이면, 잘못된 인자가 있었다는 뜻. usage 호출함
        print_help_recover();
    }
}

int command_recover_execute(char *input_src, char *input_dest, int is_recursive) {
    if (!input_src) { // <filename>이 안 주어질 때
        debug_print("filename not given\n");
        return 0;
    }

    char src_path[PATH_MAX * 2];
    get_absolute_path(input_src, src_path); // 절대 경로로 변환

    if (strlen(src_path) > PATH_MAX) {
        printf("file argument length exceed, given size : %ld\n", strlen(src_path));
        return 0;
    }

    if (!check_path_in_directory(src_path, USER_DIR)) { // path가 홈 디렉터리를 벗어나면
        printf("\"%s\" can't be recovered\n", input_src);
        return 1;
    }

    // 입력 받은 경로가 백업 디렉토리에 접근하는 경우는 2가지 중 하나
    // case 1) 유저 디렉터리와 동일한 경로
    // case 2) 백업 디렉토리 내 경로 (백업 디렉토리와 동일한 경우 포함)
    if (!strcmp(USER_DIR, src_path) || check_path_in_directory(src_path, BACKUP_DIR)) {
        printf("\"%s\" can't be recovered\n", input_src);
        return 1;
    }

    { // src_path를 백업 디렉토리 경로로 변경함
        char tmp[PATH_MAX];
        sprintf(tmp, "%s/%s", BACKUP_DIR, get_relative_path(src_path, USER_DIR));
        get_absolute_path(tmp, src_path);
    }

    char dest_path[PATH_MAX * 2];
    if (input_dest) get_absolute_path(input_dest, dest_path);
    else get_absolute_path(input_src, dest_path);

    if (strlen(dest_path) > PATH_MAX) {
        printf("file argument length exceed, given size : %ld\n", strlen(dest_path));
        return 0;
    }

    debug_print("input : %s\n", input_dest);
    debug_print("dst : %s\n", dest_path);

    // 입력 받은 경로가 백업 디렉토리에 접근하는 경우
    // case 1) 백업 디렉토리 내 경로 (백업 디렉토리와 동일한 경우 포함)
    if (!check_path_in_directory(dest_path, USER_DIR) || check_path_in_directory(dest_path, BACKUP_DIR)) {
        printf("\"%s\" can't be recovered\n", input_dest);
        return 1;
    }

    command_recover_process_file_list(src_path, dest_path, is_recursive);

    return 1;
}

void command_recover_process_file_list(const char *src_path, const char *dest_path, int is_recursive) {
    debug_print("src : %s\ndest : %s\nis_recursive : %d\n", src_path, dest_path, is_recursive);

    int found_file = command_recover_single_file(src_path, dest_path);

    { // src_path/ 형식의 디렉터리 복구를 진행한다.
        struct Node *root = find_all_files(src_path, 1, 1);
        struct Node *node = root;

        if (!root) {
            if (!found_file) { // 폴더도 파일도 없는 경우에 해당
                print_help_recover();
            }
            return;
        }

        if (!is_recursive) {
            debug_print("given directory, %s\n", src_path);
            if (!found_file) { // single file recover도 못했으므로 디렉터리 recover을 시도하는 상황
                printf("\"%s\" is a directory file, need -d option\n", src_path);
                print_help_recover();
            }
            clear_node(root);
            return;
        }

        struct Node *vis = NULL; // 이미 처리한 파일 목록 담아둠
        while (node) {
            char origin_path[PATH_MAX];
            remove_filename_date(node->path, origin_path);
            found_file = 1;

            {
                int find = 0;
                struct Node *vis_node = vis;
                while (vis_node) {
                    if (!strcmp(vis_node->path, origin_path)) {
                        find = 1;
                        break;
                    }
                    vis_node = vis_node->next;
                }
                if (find) {
                    node = node->next;
                    continue;
                }
            }

            char tmp_dest_path[PATH_MAX];
            sprintf(tmp_dest_path, "%s/%s", dest_path, get_relative_path(origin_path, src_path));

            command_recover_single_file(origin_path, tmp_dest_path); // origin_path에 있는 파일을 tmp_dest_path로 옮긴다. (delete after copy)

            if (vis == NULL) vis = create_node(origin_path);
            else get_last_node(vis)->next = create_node(origin_path);

            node = node->next;
        }

        clear_node(vis);
        clear_node(root);
    }

    if (!found_file) {
        debug_print("No files found\n");
        print_help_recover();
        exit(0);
    }
}

int command_recover_single_file(const char *backup_path, const char *dest_path) {
    struct Node *root = find_same_name_files(backup_path);
    struct Node *node = root;

    int size = get_size_linked_list(root);

    if (!root) return 0; // 파일이 없으면 함수 실행 안 함

    {
        while (node) {
            if (is_directory(node->path) < 0) { // 일반 파일이 아니면 프로그램 종료
                printf("%s is not file and directory.\n", node->path);
                clear_node(root);
                exit(0);
            }
            if (access(node->path, R_OK | W_OK) < 0) { // 권한이 없으면 프로그램 종료
                printf("%s doesn't have read/write permission.\n", node->path);
                clear_node(root);
                exit(0);
            }
            node = node->next;
        }
    }

    int recover_index = 1;

    node = root;
    if (size > 1) { // 파일이 여러 개면 프롬프트 입력을 띄움
        {
            char tmp[PATH_MAX];
            sprintf(tmp, "%s/%s", USER_DIR, get_relative_path(backup_path, BACKUP_DIR));

            printf("backup file list of \"%s\"\n", tmp);
        }

        printf("0. exit\n");
        for (int i = 0; i < size; i++) {
            printf("%d. %s\t\t%'ldbytes\n", i + 1, get_date_in_path(node->path),
                   get_filesize(node->path));

            node = node->next;
        }
        printf("Choose file to recover\n>> ");

        scanf("%d", &recover_index);

        if (recover_index < 1 || recover_index > size) {
            clear_node(root);
            exit(0); // 올바르지 않은 인덱스 입력되면 프로그램 종료함
        }
    }

    node = root;
    while (--recover_index) node = node->next;

    if (compare_file(dest_path, node->path)) { // 해시값 비교했을 때 같은 파일인가?
        printf("\"%s\" and \"%s\" are same files.\n", dest_path, node->path);
    } else {
        copy_file(dest_path, node->path); // 파일 복사
        unlink(node->path);
        printf("\"%s\" backup recover to \"%s\"\n", node->path, dest_path);
    }

    clear_node(root);
    return 1;
}
