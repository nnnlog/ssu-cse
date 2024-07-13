#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "usage/usage.h"
#include "file/file.h"
#include "file/path.h"
#include "debug/debug.h"

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); // flush buffer always

    int need_help = 0;

    if (argc == 2) {
        if (!strcmp(argv[1], "md5")) is_MD5 = 1;
        else if (strcmp(argv[1], "sha1")) need_help = 1;
    } else need_help = 1;

    if (need_help) {
        print_help_program();
        return 0;
    }

    debug_print("DEBUG : %d\n", DEBUG);
    debug_print("Use %s\n", is_MD5 ? "md5" : "sha1");

    init_filesystem(argv[0]); // 실행 파일 경로 지정

    while (1) {
        printf("20221494> ");

        char input_buffer[10000] = {0};
        if (scanf("%9999[^\n]", input_buffer) < 0) return 0; // 버퍼 크기는 10k로 잡음. recover 명령어가 <filename>과 -n에 의한 <new path> 두 개의 파일 경로를 인풋으로 받는 것이 최대인데, 이 때 필요한 버퍼 크기가 대략 4096 * 2 정도 밖에 되지 않음
        {
            char tmp = getchar(); // try to remove \n(new line) remaining in stdin.
            if (tmp < 0) return 0; // EOF
            if (tmp != '\n') {
                while ((tmp = getchar()) != '\n'); // \n이 아닌 경우, command 뒤에 buffer가 남은 것이므로 에러 처리 후 프롬프트 재출력함
                printf("exceed command buffer length\n");
                continue;
            }
        }

        if (!*input_buffer) { // 엔터만 쳤을 때
            print_help_all();
            continue;
        }

        debug_print("input string '%s'\n", input_buffer);

        char *ptr = strtok(input_buffer, " ");
        if (ptr == NULL || !*ptr) { // 공백 문자만 포함되는 등, 입력된 명령어가 정상적이지 않은 경우
            print_help_all();
            continue;
        }

        char cmd[PATH_MAX * 2];
        strcpy(cmd, ptr);

        debug_print("input command '%s'\n", cmd);

        char exec_path[PATH_MAX];
        sprintf(exec_path, "%s/", EXECUTE_DIR);

        int include_hash_info = 1;

        if (!strcmp(cmd, "add")) {
            strcat(exec_path, "command_add");
        } else if (!strcmp(cmd, "remove")) {
            strcat(exec_path, "command_remove");
        } else if (!strcmp(cmd, "recover")) {
            strcat(exec_path, "command_recover");
        } else if (!strcmp(cmd, "ls") || !strcmp(cmd, "vi") || !strcmp(cmd, "vim")) {
            char var_path[PATH_MAX];
            strcpy(var_path, getenv("PATH"));

            int ok = 0;
            char *dir = var_path;
            while (*dir && (dir = strtok(dir, ":"))) {
                char tmp[PATH_MAX];
                sprintf(tmp, "%s/%s", dir, cmd);

                if (!access(tmp, X_OK)) {
                    ok = 1;
                    strcpy(exec_path, tmp);
                    break;
                }

                dir = dir + strlen(dir) + 1;
            }

            if (!ok) {
                printf("[ERROR] cannot find \"%s\" executable path.\n", cmd);
                continue;
            }
//            strcpy(exec_path, "/bin/");
//            strcat(exec_path, cmd); // run command directly
            include_hash_info = 0;
        } else if (!strcmp(cmd, "exit")) {
            return 0;
        } else { // it includes help command
            strcat(exec_path, "command_help");
        }

        char **exec_argv = calloc(sizeof(char **), 2); // 명령어 이름 이후에 들어온 인자들을 모두 동적 할당하여 execv 호출 시 넘겨줄 argv로 만들어주는 작업
        int exec_argv_index = 0;
        exec_argv[exec_argv_index++] = exec_path;
        if (include_hash_info) exec_argv[exec_argv_index++] = is_MD5 ? "1" : "0"; // 해쉬 함수 정보 저장해서 넘겨줌

        ptr = ptr + strlen(ptr) + 1;
        while (*ptr && (ptr = strtok(ptr, " "))) {
            char *tmp = malloc(strlen(ptr) + 1);
            strcpy(tmp, ptr);

            exec_argv = reallocarray(exec_argv, sizeof(char **), exec_argv_index + 1);
            exec_argv[exec_argv_index++] = tmp;

            ptr = ptr + strlen(ptr) + 1;
        }

        exec_argv = reallocarray(exec_argv, sizeof(char **), exec_argv_index + 1);
        exec_argv[exec_argv_index++] = NULL; // argv의 마지막은 NULL이어야 하므로 명시적으로 채워줌

        debug_print("Exec path : %s\n", exec_path);

        debug_print("Exec argv : [\n");
        for (int i = 0; i < exec_argv_index; i++) {
            debug_print("%s\n", exec_argv[i]);
        }
        debug_print("]\n");

        pid_t pid = fork();
        if (!pid) { // 자식 프로세스는 fork 함수에 의한 pid 변수값이 0임
            execv(exec_path, exec_argv);
        }

        wait(NULL);

        for (int i = 1 + include_hash_info; i < exec_argv_index; i++) { // do not free exec_path, (md5|sha1)
            if (exec_argv[i]) free(exec_argv[i]);
        }
        free(exec_argv);
    }
}
