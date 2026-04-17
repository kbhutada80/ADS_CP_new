#ifndef BPTREE_H
#define BPTREE_H

#include <time.h>

/*
 * B+ Tree Header File
 * 
 * This module implements a disk-style B+ Tree for advanced key-value storage.
 * Key features:
 * - Configurable node order (fan-out)
 * - All values stored in leaf nodes
 * - Internal nodes store only keys and pointers
 * - Linked leaf nodes for efficient range queries
 */

#define BPTREE_DEFAULT_ORDER 4

typedef struct BPTreeLeafNode {
    int *keys;                          // Array of keys
    char **values;                      // Array of values
    int key_count;                      // Number of keys in this node
    struct BPTreeLeafNode *next;        // Pointer to next leaf (linked list)
    struct BPTreeLeafNode *prev;        // Pointer to previous leaf
    int *node_ids;                      // For disk simulation
} BPTreeLeafNode;

typedef struct BPTreeInternalNode {
    int *keys;                          // Array of keys
    struct BPTreeNode **children;       // Array of child pointers
    int key_count;                      // Number of keys in this node
} BPTreeInternalNode;

typedef struct BPTreeNode {
    int is_leaf;                        // 1 if leaf, 0 if internal
    union {
        BPTreeLeafNode leaf;
        BPTreeInternalNode internal;
    } data;
    int disk_reads;                     // Simulated disk read count
} BPTreeNode;

typedef struct {
    BPTreeNode *root;
    int order;                          // Node order (max keys = order - 1)
    int node_count;                     // Total nodes created
    BPTreeLeafNode *leftmost_leaf;      // Ptr to leftmost leaf for range queries
} BPTree;

/* Core B+ Tree Operations */
BPTree* bptree_create(int order);

void bptree_insert(BPTree *tree, int key, const char *value);

char* bptree_search(BPTree *tree, int key, int *disk_reads);

int bptree_delete(BPTree *tree, int key);

void bptree_print(BPTree *tree);

void bptree_destroy(BPTree *tree);

/* Utility Functions */
int bptree_height(BPTree *tree);

int bptree_node_count(BPTree *tree);

void bptree_visualize(BPTree *tree);

#endif // BPTREE_H
