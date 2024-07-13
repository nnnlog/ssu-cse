#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#include "../commons/daemon_manager.h"
#include "../commons/path.h"
#include "../commons/const.h"
#include "../commons/usage.h"

int main(int argc, char *argv[]) {
    init_filesystem();
    set_additional_path(argv[0]);

    // parse option
    if (argc < 2) {
        print_usage_add();
        exit(1);
    }
    
    char path[PATH_MAX];
    if (realpath(argv[1], path) == NULL) { // 절대 경로로 변환
        printf("(%s) doesn't exist.\n", argv[1]);
        print_usage_add();
        exit(1);
    }
    {
        struct stat statbuf;
        if (lstat(path, &statbuf) < 0) {
            printf("lstat for (%s) failed.\n", argv[1]);
            exit(1);
        }

        if (!S_ISDIR(statbuf.st_mode)) { // 디렉터리 예외 처리
            printf("(%s) is not directory.\n", argv[1]);
            print_usage_add();
            exit(1);
        }
    }

    int repeat_time = 1; // 반복 시간
    
    optind = 2;
    int option;
    while ((option = getopt(argc, argv, "t:")) != -1) {
        switch (option) {
            case 't':
                for (int i = 0; i < strlen(optarg); i++) {
                    if (!isdigit(optarg[i])) { // 모든 문자가 숫자인지 확인
                        print_usage_add();
                        exit(1);
                    }
                }
                repeat_time = atoi(optarg); // 숫자로 변환
                if (repeat_time < 0) { // 올바른 시간이 아닌 경우 예외 처리
                    printf("wrong range : %d\n", repeat_time);
                    exit(1);
                }
                break;
            default:
                print_usage_add();
                break;
        }
    }

    // run new daemon process
    if (add_daemon_process(path, repeat_time)) { // 데몬 프로세스 실행에 성공하면 다음 문장 실행
        printf("monitoring started (%s)\n", path);
    } else {
        printf("monitoring failed (%s)\n", path);
    }

    exit(0);
}
