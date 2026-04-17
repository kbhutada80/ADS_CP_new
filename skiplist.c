#include "skiplist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

/*
 * Skip List Implementation
 * 
 * A Skip List is a probabilistic data structure that allows for:
 * - O(log n) average search, insert, and delete
 * - Simpler implementation than balanced trees
 * - Useful for performance comparison with B+ Tree
 * 
 * How it works:
 * 1. Each node has multiple levels (randomly determined)
 * 2. Level 0 contains all elements
 * 3. Higher levels contain fewer elements with exponential distribution
 * 4. Search skips over elements efficiently using higher levels
 */

/* Forward declarations */
static int _random_level(void);
static SkipListNode* _create_skiplist_node(int key, const char *value, int level);
static void _free_skiplist_node(SkipListNode *node);

/*
 * Generate random level for new node
 * Probability: P = 0.5 (50% chance to increase level)
 */
static int _random_level(void) {
    int level = 1;
    while ((rand() % 2 == 0) && level < SKIPLIST_MAX_LEVEL) {
        level++;
    }
    return level;
}

/*
 * Create a new Skip List node
 */
static SkipListNode* _create_skiplist_node(int key, const char *value, int level) {
    SkipListNode *node = (SkipListNode *)malloc(sizeof(SkipListNode));
    if (!node) return NULL;

    node->key = key;
    node->value = NULL;
    if (value) {
        node->value = (char *)malloc(strlen(value) + 1);
        strcpy(node->value, value);
    }
    node->level = level;
    node->forward = (SkipListNode **)malloc(level * sizeof(SkipListNode *));
    
    for (int i = 0; i < level; i++) {
        node->forward[i] = NULL;
    }

    return node;
}

/*
 * Free a Skip List node
 */
static void _free_skiplist_node(SkipListNode *node) {
    if (!node) return;
    free(node->value);
    free(node->forward);
    free(node);
}

/*
 * Create a new Skip List
 */
SkipList* skiplist_create(void) {
    SkipList *list = (SkipList *)malloc(sizeof(SkipList));
    if (!list) return NULL;

    list->header = _create_skiplist_node(INT_MIN, NULL, SKIPLIST_MAX_LEVEL);
    list->level = 1;
    list->node_count = 0;
    list->node_accesses = 0;

    return list;
}

/*
 * Insert a key-value pair into the Skip List
 */
void skiplist_insert(SkipList *list, int key, const char *value) {
    if (!list || !value) return;

    // Create update array to track nodes that need pointer updates
    SkipListNode **update = (SkipListNode **)malloc(SKIPLIST_MAX_LEVEL * sizeof(SkipListNode *));
    SkipListNode *current = list->header;

    // Find insertion position from top level down
    for (int i = list->level - 1; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
            list->node_accesses++;
        }
        update[i] = current;
    }

    current = current->forward[0];

    // If key exists, update value
    if (current && current->key == key) {
        free(current->value);
        current->value = (char *)malloc(strlen(value) + 1);
        strcpy(current->value, value);
        free(update);
        return;
    }

    // Create new node with random level
    int new_level = _random_level();
    if (new_level > list->level) {
        for (int i = list->level; i < new_level; i++) {
            update[i] = list->header;
        }
        list->level = new_level;
    }

    SkipListNode *new_node = _create_skiplist_node(key, value, new_level);
    if (!new_node) {
        free(update);
        return;
    }

    // Update forward pointers
    for (int i = 0; i < new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    list->node_count++;
    free(update);
}

/*
 * Search for a key in the Skip List
 */
char* skiplist_search(SkipList *list, int key, int *node_accesses) {
    if (!list) return NULL;

    int accesses = 0;
    SkipListNode *current = list->header;

    // Search from top level down
    for (int i = list->level - 1; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
            accesses++;
        }
    }

    current = current->forward[0];
    accesses++;

    if (node_accesses) {
        *node_accesses = accesses;
    }

    if (current && current->key == key) {
        return current->value;
    }

    return NULL;
}

/*
 * Delete a key from the Skip List
 */
int skiplist_delete(SkipList *list, int key) {
    if (!list) return 0;

    SkipListNode **update = (SkipListNode **)malloc(SKIPLIST_MAX_LEVEL * sizeof(SkipListNode *));
    SkipListNode *current = list->header;

    // Find node to delete
    for (int i = list->level - 1; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];

    // Key not found
    if (!current || current->key != key) {
        free(update);
        return 0;
    }

    // Update pointers to skip deleted node
    for (int i = 0; i < list->level; i++) {
        if (update[i]->forward[i] != current) {
            break;
        }
        update[i]->forward[i] = current->forward[i];
    }

    _free_skiplist_node(current);
    list->node_count--;

    // Reduce list level if top level is empty
    while (list->level > 1 && !list->header->forward[list->level - 1]) {
        list->level--;
    }

    free(update);
    return 1;
}

/*
 * Print the Skip List (showing structure)
 */
void skiplist_print(SkipList *list) {
    if (!list) return;

    printf("\n=== Skip List Structure ===\n");
    printf("Max Level: %d, Nodes: %d\n\n", list->level, list->node_count);

    // Print each level
    for (int i = list->level - 1; i >= 0; i--) {
        printf("Level %d: ", i);
        SkipListNode *node = list->header->forward[i];
        while (node) {
            printf("[%d:%s] -> ", node->key, node->value);
            node = node->forward[i];
        }
        printf("NULL\n");
    }

    printf("\nAll Elements (Level 0):\n");
    SkipListNode *node = list->header->forward[0];
    while (node) {
        printf("  %d => %s\n", node->key, node->value);
        node = node->forward[0];
    }
    printf("\n");
}

/*
 * Get node count
 */
int skiplist_node_count(SkipList *list) {
    return list ? list->node_count : 0;
}

/*
 * Destroy the Skip List and free all memory
 */
void skiplist_destroy(SkipList *list) {
    if (!list) return;

    SkipListNode *node = list->header->forward[0];
    while (node) {
        SkipListNode *next = node->forward[0];
        _free_skiplist_node(node);
        node = next;
    }

    _free_skiplist_node(list->header);
    free(list);
}
