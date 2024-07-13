#include "hash.h"
#include "file.h"
#include "../debug/debug.h"

#include <stdlib.h>

#include <openssl/md5.h>
#include <openssl/sha.h>


int is_MD5 = 0; // otherwise, sha1

/// free return value
unsigned char *hash_file(const char *path) {
    int length;
    unsigned char *content = read_file(path, &length);
    if (content == NULL) return NULL;

    unsigned char *ret = malloc((is_MD5 ? MD5_DIGEST_LENGTH : SHA_DIGEST_LENGTH) + 1);
    memset(ret, 0, (is_MD5 ? MD5_DIGEST_LENGTH : SHA_DIGEST_LENGTH) + 1);

    if (is_MD5) {
        MD5(content, length, ret);
    } else {
        SHA1(content, length, ret);
    }

    free(content);

    return ret;
}

int compare_file(const char *path1, const char *path2) {
    unsigned char *hash1 = hash_file(path1);
    unsigned char *hash2 = hash_file(path2);

    if (!hash1 || !hash2) {
        if (hash1) free(hash1);
        if (hash2) free(hash2);
        return 0;
    }

    int ret = 1;
    for (int i = 0; i < (is_MD5 ? MD5_DIGEST_LENGTH : SHA_DIGEST_LENGTH); i++) {
        if (hash1[i] != hash2[i]) {
            ret = 0;
            break;
        }
    }
//    ret = !strcmp(hash1, hash2);

    free(hash1);
    free(hash2);
    return ret;
}
