#include "../file/path.h"
#include <linux/limits.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    char res[PATH_MAX];
    get_absolute_path(argv[1], res);

    printf("%s", res);
}
