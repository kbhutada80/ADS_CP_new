#include "avl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * AVL Tree Implementation
 *
 * A self-balancing BST where for every node, the height difference
 * between left and right subtrees is at most 1.
 *
 * Rebalancing uses four types of rotations:
 *   LL (Right Rotation): Left-heavy, inserted into left-left
 *   RR (Left Rotation):  Right-heavy, inserted into right-right
 *   LR (Left-Right):     Left-heavy, inserted into left-right
 *   RL (Right-Left):     Right-heavy, inserted into right-left
 */

/* ===================== Helper Functions ===================== */

static int _avl_node_height(AVLNode *node) {
    return node ? node->height : 0;
}

static int _avl_balance_factor(AVLNode *node) {
    return node ? _avl_node_height(node->left) - _avl_node_height(node->right) : 0;
}

static int _max(int a, int b) {
    return (a > b) ? a : b;
}

static void _avl_update_height(AVLNode *node) {
    if (node) {
        node->height = 1 + _max(_avl_node_height(node->left), _avl_node_height(node->right));
    }
}

static AVLNode* _avl_create_node(int key, const char *value) {
    AVLNode *node = (AVLNode *)malloc(sizeof(AVLNode));
    if (!node) return NULL;

    node->key = key;
    node->value = (char *)malloc(strlen(value) + 1);
    strcpy(node->value, value);
    node->left = NULL;
    node->right = NULL;
    node->height = 1;

    return node;
}

/* ===================== Rotations ===================== */

/*
 * Right Rotation (LL case)
 *        y              x
 *       / \            / \
 *      x   T3  =>   T1   y
 *     / \                / \
 *   T1   T2            T2   T3
 */
static AVLNode* _avl_rotate_right(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;

    x->right = y;
    y->left = T2;

    _avl_update_height(y);
    _avl_update_height(x);

    return x;
}

/*
 * Left Rotation (RR case)
 *      x                y
 *     / \              / \
 *   T1   y    =>     x   T3
 *       / \         / \
 *     T2   T3     T1   T2
 */
static AVLNode* _avl_rotate_left(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;

    y->left = x;
    x->right = T2;

    _avl_update_height(x);
    _avl_update_height(y);

    return y;
}

/* ===================== Insert ===================== */

static AVLNode* _avl_insert_node(AVLNode *node, int key, const char *value, int *count) {
    /* Standard BST insert */
    if (!node) {
        (*count)++;
        return _avl_create_node(key, value);
    }

    if (key < node->key) {
        node->left = _avl_insert_node(node->left, key, value, count);
    } else if (key > node->key) {
        node->right = _avl_insert_node(node->right, key, value, count);
    } else {
        /* Duplicate key: update value */
        free(node->value);
        node->value = (char *)malloc(strlen(value) + 1);
        strcpy(node->value, value);
        return node;
    }

    /* Update height */
    _avl_update_height(node);

    /* Get balance factor */
    int balance = _avl_balance_factor(node);

    /* LL Case */
    if (balance > 1 && key < node->left->key) {
        return _avl_rotate_right(node);
    }

    /* RR Case */
    if (balance < -1 && key > node->right->key) {
        return _avl_rotate_left(node);
    }

    /* LR Case */
    if (balance > 1 && key > node->left->key) {
        node->left = _avl_rotate_left(node->left);
        return _avl_rotate_right(node);
    }

    /* RL Case */
    if (balance < -1 && key < node->right->key) {
        node->right = _avl_rotate_right(node->right);
        return _avl_rotate_left(node);
    }

    return node;
}

/* ===================== Search ===================== */

static char* _avl_search_node(AVLNode *node, int key, int *traversals) {
    if (!node) return NULL;

    (*traversals)++;

    if (key == node->key) {
        return node->value;
    } else if (key < node->key) {
        return _avl_search_node(node->left, key, traversals);
    } else {
        return _avl_search_node(node->right, key, traversals);
    }
}

/* ===================== Delete ===================== */

static AVLNode* _avl_min_node(AVLNode *node) {
    while (node && node->left) {
        node = node->left;
    }
    return node;
}

static AVLNode* _avl_delete_node(AVLNode *node, int key, int *deleted) {
    if (!node) return NULL;

    if (key < node->key) {
        node->left = _avl_delete_node(node->left, key, deleted);
    } else if (key > node->key) {
        node->right = _avl_delete_node(node->right, key, deleted);
    } else {
        *deleted = 1;

        /* Node with one child or no child */
        if (!node->left || !node->right) {
            AVLNode *temp = node->left ? node->left : node->right;

            if (!temp) {
                /* No child */
                free(node->value);
                free(node);
                return NULL;
            } else {
                /* One child: copy child data */
                free(node->value);
                node->key = temp->key;
                node->value = temp->value;
                node->left = temp->left;
                node->right = temp->right;
                node->height = temp->height;
                free(temp);
            }
        } else {
            /* Two children: find in-order successor */
            AVLNode *succ = _avl_min_node(node->right);
            node->key = succ->key;
            free(node->value);
            node->value = (char *)malloc(strlen(succ->value) + 1);
            strcpy(node->value, succ->value);
            node->right = _avl_delete_node(node->right, succ->key, &(int){0});
        }
    }

    if (!node) return NULL;

    _avl_update_height(node);

    int balance = _avl_balance_factor(node);

    /* LL */
    if (balance > 1 && _avl_balance_factor(node->left) >= 0)
        return _avl_rotate_right(node);

    /* LR */
    if (balance > 1 && _avl_balance_factor(node->left) < 0) {
        node->left = _avl_rotate_left(node->left);
        return _avl_rotate_right(node);
    }

    /* RR */
    if (balance < -1 && _avl_balance_factor(node->right) <= 0)
        return _avl_rotate_left(node);

    /* RL */
    if (balance < -1 && _avl_balance_factor(node->right) > 0) {
        node->right = _avl_rotate_right(node->right);
        return _avl_rotate_left(node);
    }

    return node;
}

/* ===================== Range Query ===================== */

static void _avl_range_collect(AVLNode *node, int start, int end,
                               AVLRangeResult *result, int *traversals) {
    if (!node) return;

    (*traversals)++;

    /* Prune: only go left if there could be keys >= start */
    if (node->key > start) {
        _avl_range_collect(node->left, start, end, result, traversals);
    }

    /* Collect if in range */
    if (node->key >= start && node->key <= end) {
        if (result->count >= result->capacity) {
            result->capacity *= 2;
            result->keys = (int *)realloc(result->keys, result->capacity * sizeof(int));
            result->values = (char **)realloc(result->values, result->capacity * sizeof(char *));
        }
        result->keys[result->count] = node->key;
        result->values[result->count] = (char *)malloc(strlen(node->value) + 1);
        strcpy(result->values[result->count], node->value);
        result->count++;
    }

    /* Prune: only go right if there could be keys <= end */
    if (node->key < end) {
        _avl_range_collect(node->right, start, end, result, traversals);
    }
}

/* ===================== Print ===================== */

static void _avl_print_inorder(AVLNode *node, int depth) {
    if (!node) return;

    _avl_print_inorder(node->right, depth + 1);

    for (int i = 0; i < depth; i++) printf("    ");
    printf("[%d:%s (h=%d, bf=%d)]\n", node->key, node->value,
           node->height, _avl_balance_factor(node));

    _avl_print_inorder(node->left, depth + 1);
}

/* ===================== Destroy ===================== */

static void _avl_destroy_node(AVLNode *node) {
    if (!node) return;
    _avl_destroy_node(node->left);
    _avl_destroy_node(node->right);
    free(node->value);
    free(node);
}

/* ===================== Public API ===================== */

AVLTree* avl_create(void) {
    AVLTree *tree = (AVLTree *)malloc(sizeof(AVLTree));
    if (!tree) return NULL;

    tree->root = NULL;
    tree->node_count = 0;
    tree->traversal_count = 0;

    return tree;
}

void avl_insert(AVLTree *tree, int key, const char *value) {
    if (!tree || !value) return;
    tree->root = _avl_insert_node(tree->root, key, value, &tree->node_count);
}

char* avl_search(AVLTree *tree, int key, int *traversals) {
    if (!tree) return NULL;
    int trav = 0;
    char *result = _avl_search_node(tree->root, key, &trav);
    if (traversals) *traversals = trav;
    tree->traversal_count += trav;
    return result;
}

int avl_delete(AVLTree *tree, int key) {
    if (!tree) return 0;
    int deleted = 0;
    tree->root = _avl_delete_node(tree->root, key, &deleted);
    if (deleted) tree->node_count--;
    return deleted;
}

AVLRangeResult* avl_range_query(AVLTree *tree, int start, int end, int *traversals) {
    if (!tree) return NULL;

    AVLRangeResult *result = (AVLRangeResult *)malloc(sizeof(AVLRangeResult));
    if (!result) return NULL;

    result->capacity = 64;
    result->count = 0;
    result->keys = (int *)malloc(result->capacity * sizeof(int));
    result->values = (char **)malloc(result->capacity * sizeof(char *));

    int trav = 0;
    _avl_range_collect(tree->root, start, end, result, &trav);
    if (traversals) *traversals = trav;

    return result;
}

void avl_range_result_free(AVLRangeResult *result) {
    if (!result) return;
    for (int i = 0; i < result->count; i++) {
        free(result->values[i]);
    }
    free(result->keys);
    free(result->values);
    free(result);
}

void avl_print(AVLTree *tree) {
    if (!tree || !tree->root) {
        printf("\n=== AVL Tree: (empty) ===\n");
        return;
    }

    printf("\n=== AVL Tree Visualization ===\n");
    printf("Nodes: %d, Height: %d\n\n", tree->node_count, avl_height(tree));
    _avl_print_inorder(tree->root, 0);
    printf("\n");
}

int avl_height(AVLTree *tree) {
    return (tree && tree->root) ? _avl_node_height(tree->root) : 0;
}

int avl_node_count(AVLTree *tree) {
    return tree ? tree->node_count : 0;
}

void avl_destroy(AVLTree *tree) {
    if (!tree) return;
    _avl_destroy_node(tree->root);
    free(tree);
}
