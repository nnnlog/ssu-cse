#include "linked_list.h"

#include <stdlib.h>
#include <string.h>

struct Node_wrong_problems *create_linked_list_wrong_problems_node(char *str) { // 틀린 문제 목록에 대한 노드 생성
    struct Node_wrong_problems *node = malloc(sizeof(struct Node_wrong_problems));
    node->next = NULL;
    node->value = malloc(strlen(str) + 1); // 학번 크기만큼 동적할당
    strcpy(node->value, str);
    return node;
}

struct Node_wrong_problems *free_linked_list_wrong_problems_node(struct Node_wrong_problems *node) { // 노드 할당 해제
    struct Node_wrong_problems *next = node->next;
    free(node->value);
    free(node);
    return next;
}

void clear_linked_list_wrong_problems(struct Node_wrong_problems *root) { // 연결 리스트 할당 해제
    while (root) {
        root = free_linked_list_wrong_problems_node(root);
    }
}

struct Node_sort *create_linked_list_sort_node() { // 각 사람의 채점 결과에 대한 노드 생성
    struct Node_sort *node = malloc(sizeof(struct Node_sort));
    node->next = NULL;
    *node->name = 0;
    *node->data = 0;
    return node;
}

struct Node_sort *free_linked_list_sort_node(struct Node_sort *node) {
    struct Node_sort *next = node->next;
    free(node);
    return next;
}

void clear_linked_sort_problems(struct Node_sort *root) {
    while (root) {
        root = free_linked_list_sort_node(root);
    }
}
