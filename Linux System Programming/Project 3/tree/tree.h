struct TreeCommandNode {
    int remaining;
    struct TreeCommandNode *next;
};

void print_as_tree(const char *path);
