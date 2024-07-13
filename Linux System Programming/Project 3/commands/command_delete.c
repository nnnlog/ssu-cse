#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../commons/daemon_manager.h"
#include "../commons/path.h"
#include "../commons/const.h"
#include "../commons/usage.h"

int main(int argc, char *argv[]) {
    init_filesystem();

    if (argc < 2) {
        print_usage_delete();
        exit(1);
    }
    for (int i = 0; i < strlen(argv[1]); i++) {
        if (!isdigit(argv[1][i])) { // PID로 들어온 인자가 모두 숫자인지 확인
            print_usage_delete();
            exit(1);
        }
    }

    char ret[PATH_MAX];
    if (delete_daemon_process(atoi(argv[1]), ret) != NULL) { // 데몬 프로세스를 찾았는지 확인, 찾으면 ret에 모니터링했던 경로가 저장됨
        printf("monitoring ended (%s)\n", ret);
    } else {
        printf("monitoring deletion failed\n");
    }

    exit(0);
}
