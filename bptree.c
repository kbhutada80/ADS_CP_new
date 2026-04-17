/*
 * B+ Tree Implementation
 *
 * Disk-style B+ Tree: all data in leaves, linked leaf nodes for range queries.
 * Uses parent-stack tracking during descent (rather than searching for parent
 * after the fact) to enable safe, correct split propagation.
 *
 * Time Complexities:
 *   Insert:      O(log n)
 *   Delete:      O(log n)  [simplified: no node rebalancing after delete]
 *   Search:      O(log n)
 *   Range Query: O(log n + k) where k is result size
 */

#include "bptree.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===================== Node Allocation ===================== */

static BPTreeNode* _alloc_leaf(int order) {
    BPTreeNode *n = (BPTreeNode *)calloc(1, sizeof(BPTreeNode));
    if (!n) return NULL;
    n->is_leaf = 1;
    /* order slots: (order-1) max keys + 1 temporary overflow during split */
    n->data.leaf.keys     = (int *)calloc(order, sizeof(int));
    n->data.leaf.values   = (char **)calloc(order, sizeof(char *));
    n->data.leaf.node_ids = (int *)calloc(order, sizeof(int));
    n->data.leaf.key_count = 0;
    n->data.leaf.next = NULL;
    n->data.leaf.prev = NULL;
    return n;
}

static BPTreeNode* _alloc_internal(int order) {
    BPTreeNode *n = (BPTreeNode *)calloc(1, sizeof(BPTreeNode));
    if (!n) return NULL;
    n->is_leaf = 0;
    n->data.internal.keys     = (int *)calloc(order, sizeof(int));
    n->data.internal.children = (BPTreeNode **)calloc(order + 1, sizeof(BPTreeNode *));
    n->data.internal.key_count = 0;
    return n;
}

/* ===================== Tree Creation ===================== */

BPTree* bptree_create(int order) {
    if (order < 2) order = BPTREE_DEFAULT_ORDER;
    BPTree *tree = (BPTree *)calloc(1, sizeof(BPTree));
    if (!tree) return NULL;
    tree->order = order;
    tree->root  = _alloc_leaf(order);
    tree->node_count = 1;
    tree->leftmost_leaf = &tree->root->data.leaf;
    return tree;
}

/* ===================== Utility: get BPTreeNode from its embedded leaf ===================== */

static BPTreeNode* _node_from_leaf(BPTreeLeafNode *leaf) {
    return (BPTreeNode *)((char *)leaf - offsetof(BPTreeNode, data.leaf));
}

/* ===================== Search / Leaf Descent ===================== */

/*
 * Descend to the leaf that contains or should contain 'key'.
 * If path/path_idx/depth are non-NULL, records parent nodes along the way
 * for split propagation:
 *   path[i]     = pointer to the internal node at level i
 *   path_idx[i] = the child-slot index followed from path[i] downward
 *   *depth      = number of levels recorded (= tree height - 1)
 */
static BPTreeLeafNode* _find_leaf(BPTree *tree, int key,
                                   BPTreeNode **path, int *path_idx, int *depth) {
    BPTreeNode *node = tree->root;
    if (depth) *depth = 0;

    while (!node->is_leaf) {
        BPTreeInternalNode *in = &node->data.internal;
        int i = 0;
        while (i < in->key_count && key >= in->keys[i]) i++;
        if (path) {
            path[*depth]     = node;
            path_idx[*depth] = i;
            (*depth)++;
        }
        node = in->children[i];
        node->disk_reads++;
    }
    return &node->data.leaf;
}

char* bptree_search(BPTree *tree, int key, int *disk_reads) {
    if (!tree) return NULL;
    if (disk_reads) *disk_reads = 0;
    BPTreeLeafNode *leaf = _find_leaf(tree, key, NULL, NULL, NULL);
    for (int i = 0; i < leaf->key_count; i++) {
        if (leaf->keys[i] == key) return leaf->values[i];
    }
    return NULL;
}

/* ===================== Insert helpers ===================== */

/* Insert directly into a leaf that has room */
static void _leaf_insert_direct(BPTreeLeafNode *leaf, int key, const char *value) {
    int i = leaf->key_count - 1;
    while (i >= 0 && leaf->keys[i] > key) {
        leaf->keys[i + 1]   = leaf->keys[i];
        leaf->values[i + 1] = leaf->values[i];
        i--;
    }
    leaf->keys[i + 1]   = key;
    leaf->values[i + 1] = (char *)malloc(strlen(value) + 1);
    strcpy(leaf->values[i + 1], value);
    leaf->key_count++;
}

/* Forward declaration */
static void _propagate_split(BPTree *tree,
                              BPTreeNode **path, int *path_idx, int depth,
                              BPTreeNode *left, int sep_key, BPTreeNode *right);

/* ===================== Insert ===================== */

void bptree_insert(BPTree *tree, int key, const char *value) {
    if (!tree || !value) return;

    BPTreeNode *path[64];      /* parent stack; 64 levels is ample */
    int         path_idx[64];  /* child-slot taken at each level */
    int         depth = 0;

    BPTreeLeafNode *leaf = _find_leaf(tree, key, path, path_idx, &depth);

    /* Duplicate key: update value in-place */
    for (int i = 0; i < leaf->key_count; i++) {
        if (leaf->keys[i] == key) {
            free(leaf->values[i]);
            leaf->values[i] = (char *)malloc(strlen(value) + 1);
            strcpy(leaf->values[i], value);
            return;
        }
    }

    if (leaf->key_count < tree->order - 1) {
        /* Leaf has space: simple insert */
        _leaf_insert_direct(leaf, key, value);
        return;
    }

    /* ---- Leaf is full: split ---- */
    int order = tree->order;
    int max   = order - 1;  /* current key count */
    int total = max + 1;    /* after inserting the new key */

    int   *tkeys = (int *)malloc(total * sizeof(int));
    char **tvals = (char **)malloc(total * sizeof(char *));

    /* Merge existing keys + new key (sorted) into tkeys/tvals */
    int inserted = 0, j = 0;
    for (int i = 0; i < max; i++, j++) {
        if (!inserted && key < leaf->keys[i]) {
            tkeys[j] = key;
            tvals[j] = (char *)malloc(strlen(value) + 1);
            strcpy(tvals[j], value);
            j++;
            inserted = 1;
        }
        tkeys[j] = leaf->keys[i];
        tvals[j] = leaf->values[i];   /* transfer ownership */
    }
    if (!inserted) {
        tkeys[j] = key;
        tvals[j] = (char *)malloc(strlen(value) + 1);
        strcpy(tvals[j], value);
    }

    /* Split point: left keeps [0, split), right gets [split, total) */
    int split = (total + 1) / 2;  /* ceil(total/2) */

    /* Rebuild left leaf */
    leaf->key_count = split;
    for (int i = 0; i < split; i++) {
        leaf->keys[i]   = tkeys[i];
        leaf->values[i] = tvals[i];
    }

    /* Create new right leaf */
    BPTreeNode     *rnode = _alloc_leaf(order);
    BPTreeLeafNode *rleaf = &rnode->data.leaf;
    rleaf->key_count = total - split;
    for (int i = split; i < total; i++) {
        rleaf->keys[i - split]   = tkeys[i];
        rleaf->values[i - split] = tvals[i];
    }

    /* Link leaves into doubly-linked list */
    rleaf->next = leaf->next;
    rleaf->prev = leaf;
    if (leaf->next) leaf->next->prev = rleaf;
    leaf->next = rleaf;

    tree->node_count++;

    int sep = rleaf->keys[0];   /* separator key pushed to parent */

    free(tkeys);
    free(tvals);  /* values were transferred, not freed */

    BPTreeNode *lnode = _node_from_leaf(leaf);
    _propagate_split(tree, path, path_idx, depth, lnode, sep, rnode);
}

/* ===================== Split propagation ===================== */

/*
 * After a child node split, insert sep_key / right child into the parent.
 * If parent is also full, split it and propagate further up.
 */
static void _propagate_split(BPTree *tree,
                              BPTreeNode **path, int *path_idx, int depth,
                              BPTreeNode *left, int sep_key, BPTreeNode *right) {
    if (depth == 0) {
        /* Left node was the root: make a new root */
        BPTreeNode *new_root = _alloc_internal(tree->order);
        new_root->data.internal.keys[0]       = sep_key;
        new_root->data.internal.children[0]   = left;
        new_root->data.internal.children[1]   = right;
        new_root->data.internal.key_count     = 1;
        tree->root = new_root;
        tree->node_count++;
        return;
    }

    BPTreeNode         *parent = path[depth - 1];
    int                 cpos   = path_idx[depth - 1];
    BPTreeInternalNode *pin    = &parent->data.internal;
    int                 order  = tree->order;

    if (pin->key_count < order - 1) {
        /* Parent has room: shift and insert */
        for (int i = pin->key_count; i > cpos; i--) {
            pin->keys[i]         = pin->keys[i - 1];
            pin->children[i + 1] = pin->children[i];
        }
        pin->keys[cpos]         = sep_key;
        pin->children[cpos + 1] = right;
        pin->key_count++;
        return;
    }

    /* ---- Parent is full: split the internal node ---- */
    int max   = order - 1;
    int total = max + 1;   /* key count after inserting sep_key */

    int         *tkeys = (int *)malloc(total * sizeof(int));
    BPTreeNode **tchld = (BPTreeNode **)malloc((total + 1) * sizeof(BPTreeNode *));

    /*
     * Build merged temp arrays.
     * cpos is the KEY position where sep_key is inserted.
     * Right child goes at children[cpos+1].
     *
     * Existing:
     *   children[0..max]   (max+1 entries)
     *   keys[0..max-1]     (max entries)
     *
     * We produce:
     *   tchld[0..total]    (total+1 entries)
     *   tkeys[0..total-1]  (total entries)
     */
    tchld[0] = pin->children[0];
    int ti = 0;
    for (int i = 0; i < max; i++) {
        if (ti == cpos) {
            tkeys[ti]       = sep_key;
            tchld[ti + 1]   = right;
            ti++;
        }
        tkeys[ti]       = pin->keys[i];
        tchld[ti + 1]   = pin->children[i + 1];
        ti++;
    }
    if (ti == cpos) {   /* sep_key goes at the very end */
        tkeys[ti]       = sep_key;
        tchld[ti + 1]   = right;
        ti++;
    }
    /* Now ti == total */

    /* Median to push up */
    int mid     = total / 2;
    int med_key = tkeys[mid];

    /* Left internal node (reuse parent) */
    pin->key_count = mid;
    for (int i = 0; i < mid; i++) {
        pin->keys[i]     = tkeys[i];
        pin->children[i] = tchld[i];
    }
    pin->children[mid] = tchld[mid];

    /* New right internal node */
    BPTreeNode         *rnode = _alloc_internal(order);
    BPTreeInternalNode *rin   = &rnode->data.internal;
    rin->key_count = total - mid - 1;
    for (int i = 0; i < rin->key_count; i++) {
        rin->keys[i]     = tkeys[mid + 1 + i];
        rin->children[i] = tchld[mid + 1 + i];
    }
    rin->children[rin->key_count] = tchld[total];

    tree->node_count++;

    free(tkeys);
    free(tchld);

    /* Propagate median key one level up */
    _propagate_split(tree, path, path_idx, depth - 1, parent, med_key, rnode);
}

/* ===================== Delete ===================== */

/*
 * Simplified delete: find leaf and remove key.
 * No node rebalancing (underflow is tolerated).
 */
int bptree_delete(BPTree *tree, int key) {
    if (!tree) return 0;
    BPTreeLeafNode *leaf = _find_leaf(tree, key, NULL, NULL, NULL);
    for (int i = 0; i < leaf->key_count; i++) {
        if (leaf->keys[i] == key) {
            free(leaf->values[i]);
            for (int j = i; j < leaf->key_count - 1; j++) {
                leaf->keys[j]   = leaf->keys[j + 1];
                leaf->values[j] = leaf->values[j + 1];
            }
            leaf->key_count--;
            return 1;
        }
    }
    return 0;
}

/* ===================== Metrics ===================== */

int bptree_height(BPTree *tree) {
    if (!tree || !tree->root) return 0;
    int h = 0;
    BPTreeNode *n = tree->root;
    while (!n->is_leaf) { h++; n = n->data.internal.children[0]; }
    return h + 1;
}

int bptree_node_count(BPTree *tree) {
    return tree ? tree->node_count : 0;
}

/* ===================== Print / Visualize ===================== */

void bptree_print(BPTree *tree) {
    if (!tree || !tree->root) return;

    printf("\n=== B+ Tree Visualization ===\n");
    printf("Order: %d | Nodes: %d | Height: %d\n\n",
           tree->order, tree->node_count, bptree_height(tree));

    /* BFS with a dynamically grown queue */
    int cap = tree->node_count * 2 + 64;
    BPTreeNode **queue = (BPTreeNode **)malloc(cap * sizeof(BPTreeNode *));
    if (!queue) return;

    int front = 0, rear = 0;
    queue[rear++] = tree->root;
    int level = 1, lvl_cnt = 1, next_cnt = 0;

    while (front < rear) {
        BPTreeNode *node = queue[front++];

        /* Grow queue if approaching capacity */
        if (rear >= cap - (tree->order + 2)) {
            cap *= 2;
            BPTreeNode **tmp = (BPTreeNode **)realloc(queue, cap * sizeof(BPTreeNode *));
            if (!tmp) { free(queue); return; }
            queue = tmp;
        }

        printf("[L%d] ", level);
        if (node->is_leaf) {
            BPTreeLeafNode *lf = &node->data.leaf;
            printf("LEAF[");
            for (int i = 0; i < lf->key_count; i++) {
                if (i) printf(" ");
                printf("%d", lf->keys[i]);
            }
            printf("]\n");
        } else {
            BPTreeInternalNode *in = &node->data.internal;
            printf("INT[");
            for (int i = 0; i < in->key_count; i++) {
                if (i) printf(" ");
                printf("%d", in->keys[i]);
            }
            printf("]\n");
            for (int i = 0; i <= in->key_count; i++) {
                if (in->children[i]) {
                    queue[rear++] = in->children[i];
                    next_cnt++;
                }
            }
        }

        if (--lvl_cnt == 0) {
            level++;
            lvl_cnt  = next_cnt;
            next_cnt = 0;
        }
    }

    /* Show linked leaf chain (first 20 leaves) */
    printf("\nLeaf chain: ");
    BPTreeLeafNode *lf = tree->leftmost_leaf;
    int shown = 0;
    while (lf && shown < 20) {
        if (shown) printf(" -> ");
        printf("[");
        for (int i = 0; i < lf->key_count; i++) {
            if (i) printf(",");
            printf("%d", lf->keys[i]);
        }
        printf("]");
        lf = lf->next;
        shown++;
    }
    if (lf) printf(" -> ...");
    printf("\n\n");

    free(queue);
}

void bptree_visualize(BPTree *tree) {
    if (!tree) return;
    printf("\n=== B+ Tree Analysis ===\n");
    printf("Order: %d | Height: %d | Total Nodes: %d | Max Keys/Node: %d\n",
           tree->order, bptree_height(tree), tree->node_count, tree->order - 1);
    bptree_print(tree);
}

/* ===================== Destroy ===================== */

static void _destroy_node(BPTreeNode *node) {
    if (!node) return;
    if (node->is_leaf) {
        BPTreeLeafNode *lf = &node->data.leaf;
        for (int i = 0; i < lf->key_count; i++) free(lf->values[i]);
        free(lf->keys);
        free(lf->values);
        free(lf->node_ids);
    } else {
        BPTreeInternalNode *in = &node->data.internal;
        for (int i = 0; i <= in->key_count; i++)
            _destroy_node(in->children[i]);
        free(in->keys);
        free(in->children);
    }
    free(node);
}

void bptree_destroy(BPTree *tree) {
    if (!tree) return;
    _destroy_node(tree->root);
    free(tree);
}
