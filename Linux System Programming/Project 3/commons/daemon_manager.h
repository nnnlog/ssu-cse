#include <sys/types.h>
#include <linux/limits.h>

#include "path.h"

extern void init_filesystem();

extern int exist_path_exactly(const char *path);

extern int add_daemon_process(const char *path, int repeat_time);

extern char *delete_daemon_process(pid_t pid, char *resolved);
