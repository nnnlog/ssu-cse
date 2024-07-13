#include <linux/limits.h>
#include <sys/types.h>
#include <stdio.h>

struct DaemonFileNode {
    time_t mtime;
    char *path;

    /** use when list_cnt > 0 */
    int list_cnt;
    struct DaemonFileNode **list;
};

extern void log_file(FILE *fp, time_t tm, const char *head, const char *body);

extern struct DaemonFileNode *construct_tree(const char *path);

extern void free_tree(struct DaemonFileNode *node);

extern void compare_tree(struct DaemonFileNode *prev, struct DaemonFileNode *next, FILE *fp, const char *logPath);
