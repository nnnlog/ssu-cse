#include "debug.h"

#include <stdio.h>
#include <string.h>

#if DEBUG == 1
void debug_hash(const unsigned char *hash, int length) {
    debug_print(" HASH : ");
    for (int i = 0; i < length; i++) {
        printf("%02x", hash[i]); // 16진수 2자리(0으로 padding)로 출력
    }
    printf("\n");
}
#else
// DEBUG=0일 때는 호출되더라도 아무 작업 안 함
void debug_hash(const unsigned char *hash, int length) {

}

#endif