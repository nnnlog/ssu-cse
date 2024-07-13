#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../commons/daemon_manager.h"
#include "../commons/path.h"
#include "../commons/const.h"
#include "../commons/usage.h"
#include "../tree/tree.h"

int main(int argc, char *argv[]) {
    init_filesystem();

    if (argc < 2) {
        print_usage_tree();
        exit(1);
    }

    char path[PATH_MAX];
    if (realpath(argv[1], path) == NULL) {
        printf("(%s) doesn't exist.\n", argv[1]);
        exit(1);
    }

    if (!exist_path_exactly(path)) { // 정확히 이 경로와 일치하는 프로세스가 있는지 확인
        printf("(%s) doesn't exist in (%s).\n", path, MANAGER_TXT_PATH);
        exit(1);
    }

    print_as_tree(path); // 트리로 출력

    exit(0);
}
