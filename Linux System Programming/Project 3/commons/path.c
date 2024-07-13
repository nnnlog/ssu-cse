#include "path.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "const.h"

void set_additional_path(const char *executable_path) {
    char *buf = malloc(BUFFER_MAX); // 반드시 동적 할당 해야 함..
    char tmp[BUFFER_MAX];
    strcpy(buf, getenv("PATH"));

    strcat(buf, ":");
    strcat(buf, getcwd(tmp, BUFFER_MAX)); // 현재 디렉토리 PATH에 넣음

    realpath(executable_path, tmp);
    for (int i = strlen(tmp); i >= -1; i--) {
        if (i == -1) {
            *tmp = 0;
            break;
        }
        if (tmp[i] == '/') {
            tmp[i] = 0;
            break;
        }
    }
    strcat(buf, ":");
    strcat(buf, tmp); // 실행 파일의 디렉터리도 PATH에 넣음

    setenv("PATH", buf, 1);
}
