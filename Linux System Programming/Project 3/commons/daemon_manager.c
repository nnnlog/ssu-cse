#include "daemon_manager.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "const.h"

char MANAGER_TXT_PATH[PATH_MAX];

int can_add_path(const char *path);

char *exist_pid(pid_t pid, char *resolved);

void append_path(const char *path, pid_t pid);

void remove_pid(pid_t pid);

/*
 * monitor_list.txt 생성, 경로 저장 작업 수행
 */
void init_filesystem() {
    getcwd(MANAGER_TXT_PATH, PATH_MAX);
    strcat(MANAGER_TXT_PATH, "/monitor_list.txt");

    if (access(MANAGER_TXT_PATH, F_OK)) {
        int fd = creat(MANAGER_TXT_PATH, 0644);
        if (fd < 0) {
            fprintf(stderr, "creat for %s failed\n", MANAGER_TXT_PATH);
            exit(1);
        }
        close(fd);
    }
}

int add_daemon_process(const char *path, int repeat_time) {
    if (!can_add_path(path)) return 0;

    pid_t pid = fork();
    if (!pid) {
        // child process(daemon) section
        setsid(); // make process daemon

        char tmp1[PATH_MAX];
        strcpy(tmp1, path);

        char tmp2[PATH_MAX];
        sprintf(tmp2, "%d", repeat_time);

        char *const argv[] = {"daemon_process", tmp1, tmp2, 0}; // 데몬 프로세스 인자 생성
        execvp(*argv, argv);
//        printf("fail to create daemon process (%s)\n", strerror(errno));
        exit(1); // force kill
    } else if (pid < 0) return 0; // fork fail
    else {
        // parent process section
        append_path(path, pid); // txt 파일의 마지막에 현재 정보 저장
    }

    return 1;
}

char *delete_daemon_process(pid_t pid, char *resolved) {
    if (!exist_pid(pid, resolved)) return NULL; // pid가 존재하지 않으면 시그널 보내지 않음
    kill(pid, SIGUSR1); // 종료
    remove_pid(pid); // txt 파일에서 pid 삭제
    return resolved;
}

int exist_path_exactly(const char *path) {
    FILE *fp = fopen(MANAGER_TXT_PATH, "r");
    if (fp == NULL) return 0;

    while (!feof(fp)) {
        char curr_name[BUFFER_MAX];
        pid_t curr_pid;
        if (fscanf(fp, "%s %d\n", curr_name, &curr_pid) < 1) break;
        if (!strcmp(path, curr_name)) { // 정확히 일치하면 1을 리턴함
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

int can_add_path(const char *path) {
    FILE *fp = fopen(MANAGER_TXT_PATH, "r");
    if (fp == NULL) return 0;

    char buf[BUFFER_MAX];
    strcpy(buf, path);
    strcat(buf, "/"); // "/"을 붙여줘야 strstr 가지고 비교할 때 제대로 할 수 있음

    while (!feof(fp)) {
        char curr_name[BUFFER_MAX];
        pid_t curr_pid;
        if (fscanf(fp, "%s %d\n", curr_name, &curr_pid) < 1) break;
        strcat(curr_name, "/");
        if (strstr(curr_name, buf) || strstr(buf, curr_name)) { // 하나라도 포함 관계에 있으면 0을 리턴함
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return 1;
}

char *exist_pid(pid_t pid, char *resolved) {
    FILE *fp = fopen(MANAGER_TXT_PATH, "r");
    if (fp == NULL) return NULL;
    rewind(fp);

    while (!feof(fp)) {
        char curr_name[BUFFER_MAX];
        pid_t curr_pid;
        if (fscanf(fp, "%s %d\n", curr_name, &curr_pid) < 1) break;
        if (curr_pid == pid) { // pid가 같은지 확인, 같으면 현재 경로를 resolved에 저장
            fclose(fp);
            strcpy(resolved, curr_name);
            return resolved;
        }
    }

    fclose(fp);
    return NULL;
}

void append_path(const char *path, pid_t pid) {
    FILE *fp = fopen(MANAGER_TXT_PATH, "a"); // append로 열어서 가장 마지막에 쓴다
    if (fp == NULL) return;

    char buf[BUFFER_MAX];
    sprintf(buf, "%s %d\n", path, pid); // 먼저 문자열로 만들고 fputs로 파일에 작성

    fputs(buf, fp);
    fclose(fp);
}

void remove_pid(pid_t pid) {
    FILE *fp = fopen(MANAGER_TXT_PATH, "r");
    if (fp == NULL) return;

    int tmp_fd[2];
    pipe(tmp_fd); // pipe를 만들어서 여기에 파일 내용 임시 저장

    while (!feof(fp)) {
        char buf[BUFFER_MAX];
        if (fgets(buf, sizeof(buf), fp) == NULL) break;

        char curr_name[BUFFER_MAX];
        pid_t curr_pid;
        if (sscanf(buf, "%s %d\n", curr_name, &curr_pid) < 1) break;

        if (curr_pid != pid) { // pid가 다르면 유지해야 하는 줄의 정보이므로 pipe에 작성함
            write(tmp_fd[1], buf, strlen(buf));
        }
    }

    fclose(fp);
    close(tmp_fd[1]); // pipe 닫아줘야 read에서 EOF에 걸려서 종료될 수 있음

    fp = fopen(MANAGER_TXT_PATH, "w"); // clear file content
    if (fp == NULL) return;

    char buf[BUFFER_MAX];
    ssize_t sz;
    while ((sz = read(tmp_fd[0], buf, sizeof(buf))) > 0) fwrite(buf, sz, 1, fp); // 끝까지 buffer 단위로 읽어들임

    fclose(fp);
    close(tmp_fd[0]); // read pipe도 닫아줌
}
