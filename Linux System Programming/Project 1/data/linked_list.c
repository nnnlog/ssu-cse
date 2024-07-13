#include "linked_list.h"

struct Node *create_node(const char *path) {
    struct Node *node = malloc(sizeof(struct Node));
//    memset(node, 0, sizeof(struct Node));
    strcpy(node->path, path);
    node->next = NULL;
    return node;
}

int get_size_linked_list(struct Node *node) {
    int ret = 0;
    while (node) ret++, node = node->next; // 반복문을 통과하는 개수가 링크드 리스트의 길이와 동치임
    return ret;
}

struct Node *get_last_node(struct Node *node) {
    if (node == NULL) return NULL;
    while (node->next) node = node->next; // node->next가 NULL인 그러한 node가 가장 마지막 노드
    return node;
}

void clear_node(struct Node *root) {
    while (root) {
        struct Node *next = root->next;
        free(root);
        root = next;
    }
}
