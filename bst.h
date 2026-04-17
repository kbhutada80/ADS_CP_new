#ifndef BST_H
#define BST_H

/*
 * Binary Search Tree (BST) Header File
 *
 * Basic unbalanced Binary Search Tree implementation.
 * Included as a baseline to demonstrate the importance of balancing.
 *
 * Features:
 * - Simple structure, easy to implement
 * - Degrades to O(n) for sorted input (becomes a linked list)
 * - Range queries via in-order traversal
 *
 * Time Complexities:
 *   Insert:      O(log n) best/avg, O(n) worst (sorted input)
 *   Search:      O(log n) best/avg, O(n) worst
 *   Delete:      O(log n) best/avg, O(n) worst
 *   Range Query: O(n) worst case
 *
 * Space Complexity: O(n)
 */

typedef struct BSTNode {
    int key;
    char *value;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

typedef struct {
    BSTNode *root;
    int node_count;
    int traversal_count;  /* Track traversals for benchmarking */
} BSTree;

/* Core BST Operations */
BSTree* bst_create(void);
void bst_insert(BSTree *tree, int key, const char *value);
char* bst_search(BSTree *tree, int key, int *traversals);
int bst_delete(BSTree *tree, int key);
void bst_print(BSTree *tree);
void bst_destroy(BSTree *tree);

/* Range Query */
typedef struct {
    int *keys;
    char **values;
    int count;
    int capacity;
} BSTRangeResult;

BSTRangeResult* bst_range_query(BSTree *tree, int start, int end, int *traversals);
void bst_range_result_free(BSTRangeResult *result);

/* Utility Functions */
int bst_height(BSTree *tree);
int bst_node_count(BSTree *tree);

#endif /* BST_H */
