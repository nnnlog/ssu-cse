struct Node_wrong_problems { // 틀린 문제 목록에 대한 노드
    struct Node_wrong_problems *next;
    char *value; // 틀린 문제 데이터
};

extern struct Node_wrong_problems *create_linked_list_wrong_problems_node(char *str);

extern struct Node_wrong_problems *free_linked_list_wrong_problems_node(struct Node_wrong_problems *node);

extern void clear_linked_list_wrong_problems(struct Node_wrong_problems *root);

struct Node_sort { // 각 사람의 채점 결과에 대한 노드
    struct Node_sort *next;
    char name[1024]; // 학번
    double score; // 학생이 득한 점수
    char data[1024]; // 파일 데이터
};

extern struct Node_sort *create_linked_list_sort_node();

extern struct Node_sort *free_linked_list_sort_node(struct Node_sort *node);

extern void clear_linked_sort_problems(struct Node_sort *root);
