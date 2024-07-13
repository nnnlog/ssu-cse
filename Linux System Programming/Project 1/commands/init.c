#include "init.h"

#include <locale.h>
#include <getopt.h>

#include "../file/file.h"

void init(int argc, char *argv[]) {
    init_filesystem(argv[0]);
    is_MD5 = atoi(argv[1]) ? 1 : 0; // 인자 1이면 MD5 아니면 sha1

    setlocale(LC_ALL, ""); // 천 단위로 ' 찍기 위함
    opterr = 0; // 잘못된 인자 입력해도 getopt.h에 의한 오류 메세지 띄우지 않음
}
