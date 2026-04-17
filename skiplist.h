#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <time.h>

/*
 * Skip List Header File
 * 
 * This module implements a Skip List for comparison with B+ Tree.
 * Features:
 * - Randomized levels (probabilistic data structure)
 * - O(log n) average search/insert/delete
 * - Useful for performance comparison
 */

#define SKIPLIST_MAX_LEVEL 16
#define SKIPLIST_P 0.5                  // Probability for level generation

typedef struct SkipListNode {
    int key;                            // Key stored in this node
    char *value;                        // Value associated with key
    struct SkipListNode **forward;      // Array of pointers to next nodes at each level
    int level;                          // Level of this node
} SkipListNode;

typedef struct {
    SkipListNode *header;               // Sentinel header node
    int level;                          // Current maximum level
    int node_count;                     // Total nodes in the list
    int node_accesses;                  // Track traversals for analysis
} SkipList;

/* Core Skip List Operations */
SkipList* skiplist_create(void);

void skiplist_insert(SkipList *list, int key, const char *value);

char* skiplist_search(SkipList *list, int key, int *node_accesses);

int skiplist_delete(SkipList *list, int key);

void skiplist_print(SkipList *list);

void skiplist_destroy(SkipList *list);

/* Utility Functions */
int skiplist_node_count(SkipList *list);

#endif // SKIPLIST_H
