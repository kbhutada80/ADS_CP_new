#include "bst.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Binary Search Tree (BST) Implementation
 *
 * Basic unbalanced BST used as a baseline for comparison.
 * Demonstrates the worst-case behavior of unbalanced trees:
 *   - Sorted input causes the tree to degenerate into a linked list
 *   - Height becomes O(n) instead of O(log n)
 *   - All operations degrade to O(n)
 *
 * This implementation uses ITERATIVE algorithms for insert, search,
 * and destroy to avoid stack overflow with sorted (deep) inputs.
 */

/* ===================== Node Creation ===================== */

static BSTNode* _bst_create_node(int key, const char *value) {
    BSTNode *node = (BSTNode *)malloc(sizeof(BSTNode));
    if (!node) return NULL;
    node->key   = key;
    node->value = (char *)malloc(strlen(value) + 1);
    strcpy(node->value, value);
    node->left  = NULL;
    node->right = NULL;
    return node;
}

/* ===================== Insert (iterative) ===================== */

void bst_insert(BSTree *tree, int key, const char *value) {
    if (!tree || !value) return;

    BSTNode *new_node = _bst_create_node(key, value);
    if (!new_node) return;

    if (!tree->root) {
        tree->root = new_node;
        tree->node_count++;
        return;
    }

    BSTNode *cur = tree->root;
    while (1) {
        if (key < cur->key) {
            if (!cur->left) {
                cur->left = new_node;
                tree->node_count++;
                return;
            }
            cur = cur->left;
        } else if (key > cur->key) {
            if (!cur->right) {
                cur->right = new_node;
                tree->node_count++;
                return;
            }
            cur = cur->right;
        } else {
            /* Duplicate: update value */
            free(cur->value);
            cur->value = new_node->value;
            new_node->value = NULL;
            free(new_node);
            return;
        }
    }
}

/* ===================== Search (iterative) ===================== */

char* bst_search(BSTree *tree, int key, int *traversals) {
    if (!tree) return NULL;
    int trav = 0;
    BSTNode *cur = tree->root;
    while (cur) {
        trav++;
        if      (key < cur->key) cur = cur->left;
        else if (key > cur->key) cur = cur->right;
        else {
            if (traversals) *traversals = trav;
            tree->traversal_count += trav;
            return cur->value;
        }
    }
    if (traversals) *traversals = trav;
    tree->traversal_count += trav;
    return NULL;
}

/* ===================== Delete (iterative) ===================== */

int bst_delete(BSTree *tree, int key) {
    if (!tree || !tree->root) return 0;

    BSTNode *parent = NULL;
    BSTNode *cur    = tree->root;
    int is_left     = 0;

    /* Find the node */
    while (cur && cur->key != key) {
        parent  = cur;
        if (key < cur->key) { is_left = 1; cur = cur->left; }
        else                 { is_left = 0; cur = cur->right; }
    }
    if (!cur) return 0;  /* not found */

    BSTNode *replacement;

    if (!cur->left && !cur->right) {
        replacement = NULL;
    } else if (!cur->left) {
        replacement = cur->right;
    } else if (!cur->right) {
        replacement = cur->left;
    } else {
        /* Two children: find in-order successor (min of right subtree) */
        BSTNode *succ_parent = cur;
        BSTNode *succ        = cur->right;
        while (succ->left) {
            succ_parent = succ;
            succ        = succ->left;
        }
        /* Copy successor data into cur */
        cur->key = succ->key;
        free(cur->value);
        cur->value = (char *)malloc(strlen(succ->value) + 1);
        strcpy(cur->value, succ->value);

        /* Delete successor node (it has at most a right child) */
        BSTNode *succ_child = succ->right;
        if (succ_parent == cur)
            succ_parent->right = succ_child;
        else
            succ_parent->left  = succ_child;

        free(succ->value);
        free(succ);
        tree->node_count--;
        return 1;
    }

    /* Reconnect parent */
    if (!parent)          tree->root      = replacement;
    else if (is_left)     parent->left    = replacement;
    else                  parent->right   = replacement;

    free(cur->value);
    free(cur);
    tree->node_count--;
    return 1;
}

/* ===================== Range Query (iterative) ===================== */

/*
 * Iterative in-order traversal using an explicit stack.
 * Collects all keys in [start, end].
 */
BSTRangeResult* bst_range_query(BSTree *tree, int start, int end, int *traversals) {
    if (!tree) return NULL;

    BSTRangeResult *result = (BSTRangeResult *)malloc(sizeof(BSTRangeResult));
    if (!result) return NULL;

    result->capacity = 64;
    result->count    = 0;
    result->keys     = (int *)malloc(result->capacity * sizeof(int));
    result->values   = (char **)malloc(result->capacity * sizeof(char *));

    int trav = 0;

    /* Explicit stack for iterative in-order */
    int stack_cap = 128;
    BSTNode **stack = (BSTNode **)malloc(stack_cap * sizeof(BSTNode *));
    int top = 0;
    BSTNode *cur = tree->root;

    while (cur || top > 0) {
        /* Push left spine (but skip if all keys are too large) */
        while (cur) {
            trav++;
            if (top >= stack_cap) {
                stack_cap *= 2;
                stack = (BSTNode **)realloc(stack, stack_cap * sizeof(BSTNode *));
            }
            stack[top++] = cur;
            /* Prune: don't go left if current key <= start (all left keys < start) */
            if (cur->key <= start)
                break;
            cur = cur->left;
        }
        if (top == 0) break;
        cur = stack[--top];

        if (cur->key >= start && cur->key <= end) {
            if (result->count >= result->capacity) {
                result->capacity *= 2;
                result->keys   = (int *)realloc(result->keys,   result->capacity * sizeof(int));
                result->values = (char **)realloc(result->values, result->capacity * sizeof(char *));
            }
            result->keys[result->count]   = cur->key;
            result->values[result->count] = (char *)malloc(strlen(cur->value) + 1);
            strcpy(result->values[result->count], cur->value);
            result->count++;
        }

        if (cur->key > end) {
            /* All subsequent keys in right subtree are also > end */
            break;
        }

        cur = cur->right;
    }

    free(stack);
    if (traversals) *traversals = trav;
    return result;
}

void bst_range_result_free(BSTRangeResult *result) {
    if (!result) return;
    for (int i = 0; i < result->count; i++) free(result->values[i]);
    free(result->keys);
    free(result->values);
    free(result);
}

/* ===================== Height (iterative BFS) ===================== */

int bst_height(BSTree *tree) {
    if (!tree || !tree->root) return 0;

    /* BFS level count */
    int cap = 1024;
    BSTNode **queue = (BSTNode **)malloc(cap * sizeof(BSTNode *));
    int front = 0, rear = 0;
    queue[rear++] = tree->root;
    int height = 0;

    while (front < rear) {
        int level_size = rear - front;
        height++;
        while (level_size-- > 0) {
            BSTNode *n = queue[front++];
            if (rear + 2 >= cap) {
                cap *= 2;
                queue = (BSTNode **)realloc(queue, cap * sizeof(BSTNode *));
            }
            if (n->left)  queue[rear++] = n->left;
            if (n->right) queue[rear++] = n->right;
        }
    }
    free(queue);
    return height;
}

/* ===================== Destroy (iterative post-order) ===================== */

/*
 * Iterative destroy using a two-stack post-order traversal
 * to avoid stack overflow on degenerate (sorted-input) trees.
 */
static void _bst_destroy_node(BSTNode *root) {
    if (!root) return;

    /* Use a single stack: push node, then process children before freeing.
     * Trick: reverse post-order = modified pre-order then reverse.
     * Simpler: use plain iterative with explicit stack, process leaves first. */

    /* We'll simply do iterative in-order and collect all nodes, then free. */
    int cap = 1024;
    BSTNode **stk = (BSTNode **)malloc(cap * sizeof(BSTNode *));
    int top = 0;
    BSTNode *cur = root;

    /* Collect all nodes via iterative in-order */
    while (cur || top > 0) {
        while (cur) {
            if (top >= cap) { cap *= 2; stk = (BSTNode **)realloc(stk, cap * sizeof(BSTNode *)); }
            stk[top++] = cur;
            cur = cur->left;
        }
        cur = stk[--top];
        BSTNode *next = cur->right;
        free(cur->value);
        free(cur);
        cur = next;
    }
    free(stk);
}

/* ===================== Print ===================== */

static void _bst_print_helper(BSTNode *root, int max_nodes) {
    if (!root) return;
    int cap = 512;
    int printed = 0;

    /* Use explicit stack with (node, depth) — sideways pre-order traversal */
    typedef struct { BSTNode *n; int d; } FD;
    FD *stk = (FD *)malloc(cap * sizeof(FD));
    int top = 0;

    stk[top++] = (FD){root, 0};
    while (top > 0 && printed < max_nodes) {
        FD fd = stk[--top];
        for (int i = 0; i < fd.d; i++) printf("    ");
        printf("[%d:%s]\n", fd.n->key, fd.n->value);
        printed++;
        if (top + 2 >= cap) { cap *= 2; stk = (FD *)realloc(stk, cap * sizeof(FD)); }
        if (fd.n->right) stk[top++] = (FD){fd.n->right, fd.d + 1};
        if (fd.n->left)  stk[top++] = (FD){fd.n->left,  fd.d + 1};
    }
    if (printed >= max_nodes) printf("  ... (output capped)\n");

    free(stk);
}

void bst_print(BSTree *tree) {
    if (!tree || !tree->root) {
        printf("\n=== BST: (empty) ===\n");
        return;
    }
    printf("\n=== Binary Search Tree Visualization ===\n");
    printf("Nodes: %d, Height: %d\n\n", tree->node_count, bst_height(tree));
    _bst_print_helper(tree->root, 50);
    printf("\n");
}

/* ===================== Public API ===================== */

BSTree* bst_create(void) {
    BSTree *tree = (BSTree *)malloc(sizeof(BSTree));
    if (!tree) return NULL;
    tree->root           = NULL;
    tree->node_count     = 0;
    tree->traversal_count = 0;
    return tree;
}

int bst_node_count(BSTree *tree) {
    return tree ? tree->node_count : 0;
}

void bst_destroy(BSTree *tree) {
    if (!tree) return;
    _bst_destroy_node(tree->root);
    free(tree);
}
