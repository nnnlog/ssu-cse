#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>

#ifndef __LINKED_LIST
#define __LINKED_LIST

struct Node {
    char path[PATH_MAX];
    struct Node *next;
};
#endif

extern struct Node *create_node(const char *path);

extern struct Node *get_last_node(struct Node *node);

extern void clear_node(struct Node *root);

extern int get_size_linked_list(struct Node *node);
