#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>
#include <sys/utsname.h>

//struct utsname{};

#define MAX_BUFFER 1024

int ssu_daemon_init();

int main() {
	struct utsname uts_buf;
	time_t now;

	printf("daemon process initialization\n");
	ssu_daemon_init();

	openlog(0, 0, 0);
	syslog(LOG_ERR, "My pid is %d\n", getpid());
	closelog();

	exit(0);
}

int ssu_daemon_init() {
	pid_t pid;
	int fd, maxfd;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	} else if (pid != 0) exit(0);

	setsid();
	chdir("/");
	umask(0);
	for (int i = 0; i < getdtablesize(); i++) close(i);
	if ((fd = open("/dev/null", O_RDWR)) < 0) {
		exit(1);
	}
	dup(0);
	dup(0);

	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

	return 0;
}
