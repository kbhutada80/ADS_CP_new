#ifndef AVL_H
#define AVL_H

/*
 * AVL Tree Header File
 *
 * Self-balancing Binary Search Tree where the height difference
 * between left and right subtrees (balance factor) is at most 1.
 *
 * Features:
 * - Automatic rebalancing via rotations (LL, RR, LR, RL)
 * - Guaranteed O(log n) height
 * - Efficient range queries via in-order traversal
 *
 * Time Complexities:
 *   Insert:      O(log n) best/avg/worst
 *   Search:      O(log n) best/avg/worst
 *   Delete:      O(log n) best/avg/worst
 *   Range Query: O(log n + k) where k = result count
 *
 * Space Complexity: O(n)
 */

typedef struct AVLNode {
    int key;
    char *value;
    struct AVLNode *left;
    struct AVLNode *right;
    int height;
} AVLNode;

typedef struct {
    AVLNode *root;
    int node_count;
    int traversal_count;  /* Track traversals for benchmarking */
} AVLTree;

/* Core AVL Tree Operations */
AVLTree* avl_create(void);
void avl_insert(AVLTree *tree, int key, const char *value);
char* avl_search(AVLTree *tree, int key, int *traversals);
int avl_delete(AVLTree *tree, int key);
void avl_print(AVLTree *tree);
void avl_destroy(AVLTree *tree);

/* Range Query */
typedef struct {
    int *keys;
    char **values;
    int count;
    int capacity;
} AVLRangeResult;

AVLRangeResult* avl_range_query(AVLTree *tree, int start, int end, int *traversals);
void avl_range_result_free(AVLRangeResult *result);

/* Utility Functions */
int avl_height(AVLTree *tree);
int avl_node_count(AVLTree *tree);

#endif /* AVL_H */
