#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include "../daemon/daemon_tree.h"

FILE *fp;

void stop_by_signal(int sig) {
    if (sig == SIGUSR1) {
//        log_file(fp, "exit", NULL); // exit 출력 후 종료
        fclose(fp);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    for (int i = 0; i <= 2; i++) close(i); // daemon_process is executed by execv, hence there is not opened file except for standard i/o.
    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    umask(0);
    chdir("/");


    signal(SIGUSR1, stop_by_signal); // register handler for SIGUSR1

    char path[PATH_MAX];
    strcpy(path, argv[1]); // 모니터링할 디렉터리 위치

    char log_path[FILENAME_MAX];
    sprintf(log_path, "%s/%s", path, "log.txt"); // 로그 파일 위치
    if ((fp = fopen(log_path, "a")) == NULL) {
        exit(1);
    }
    setvbuf(fp, NULL, _IONBF, 0); // 파일은 기본적으로 full buffer이므로 line buffer나 null buffer로 변경해줘야 함

    int repeat_time = atoi(argv[2]);
    struct DaemonFileNode *prev = construct_tree(path); // 이전 트리

    do {
        struct DaemonFileNode *next = construct_tree(path); // 현재 트리
        compare_tree(prev, next, fp, log_path); // 트리 비교
        free_tree(prev); // memory leak 주의할 것
        prev = next;

        sleep(repeat_time);
    } while (1);
}
