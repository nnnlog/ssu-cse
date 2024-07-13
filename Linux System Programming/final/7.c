#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>

static void ssu_signal_handler1(int signo);
static void ssu_signal_handler2(int signo);

int main() {
	struct sigaction act_int, act_quit;

	act_int.sa_flags = 0;
	sigemptyset(&act_int.sa_mask);
	sigaddset(&act_int.sa_mask, SIGQUIT);
	act_int.sa_handler = ssu_signal_handler1;
	sigaction(SIGINT, &act_int, NULL);

	act_quit.sa_flags = 0;
	sigemptyset(&act_quit.sa_mask);
	sigaddset(&act_quit.sa_mask, SIGINT);
	act_quit.sa_handler = ssu_signal_handler2;
	sigaction(SIGQUIT, &act_quit, NULL);

	sleep(3);

	exit(0);
}

static void ssu_signal_handler1(int signo) {
	printf("Signal handler of SIGINT : %d\n", signo);
	printf("SIGQUIT signal is blocked : %d\n", signo);
	printf("sleeping 3 sec\n");
	sleep(3);
	printf("Signal handler of SIGINT ended\n");
}

static void ssu_signal_handler2(int signo) {
	printf("Signal handler of SIGQUIT : %d\n", signo);
	printf("SIGINT signal is blocked : %d\n", signo);
	printf("sleeping 3 sec\n");
	sleep(3);
	printf("Signal handler of SIGQUIT ended\n");
}
