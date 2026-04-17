#include "adaptive.h"
#include "bptree.h"
#include "skiplist.h"
#include "avl.h"
#include "bst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Adaptive Data Structure Selection System
 *
 * Uses a weighted scoring model to select the best data structure
 * for a given workload profile. Each data structure receives a score
 * based on how well it matches the workload characteristics.
 *
 * Scoring dimensions:
 *   1. Range query performance   (weighted by range_ratio)
 *   2. Insert performance        (weighted by insert_ratio)
 *   3. Search performance        (weighted by search_ratio)
 *   4. Data ordering sensitivity (based on data_randomness)
 *   5. Dataset size suitability  (based on dataset_size)
 *
 * Each dimension contributes a score from 0..10 per data structure.
 * The data structure with the highest weighted total is selected.
 */

/* ===================== Name Lookup ===================== */

static const char *DS_NAMES[] = {
    "B+ Tree",
    "Skip List",
    "AVL Tree",
    "Binary Search Tree (BST)"
};

const char* ds_type_name(DataStructureType type) {
    if (type >= 0 && type < DS_COUNT) return DS_NAMES[type];
    return "Unknown";
}

/* ===================== Selection Logic ===================== */

SelectionResult select_data_structure(WorkloadProfile profile) {
    SelectionResult result;
    memset(&result, 0, sizeof(result));

    double scores[DS_COUNT] = {0};

    /*
     * Dimension 1: Range Query Performance
     * B+ Tree excels due to linked leaf nodes (sequential scan)
     * Skip List: decent (follow level-0 links)
     * AVL/BST: in-order traversal, more overhead
     */
    double range_scores[DS_COUNT] = {10.0, 6.0, 5.0, 3.0};
    for (int i = 0; i < DS_COUNT; i++) {
        scores[i] += range_scores[i] * profile.range_ratio * 3.0;  /* Extra weight */
    }

    /*
     * Dimension 2: Insert Performance
     * Skip List: fast due to probabilistic structure, no rotations
     * BST: fast for random data (no rebalancing overhead)
     * AVL: rebalancing adds constant overhead
     * B+ Tree: node splits add overhead but amortized O(log n)
     */
    double insert_random_scores[DS_COUNT] = {7.0, 9.0, 7.0, 8.0};
    double insert_sorted_scores[DS_COUNT] = {7.0, 7.0, 6.0, 1.0};  /* BST degrades badly */

    for (int i = 0; i < DS_COUNT; i++) {
        double insert_score = insert_random_scores[i] * profile.data_randomness +
                              insert_sorted_scores[i] * (1.0 - profile.data_randomness);
        scores[i] += insert_score * profile.insert_ratio * 2.0;
    }

    /*
     * Dimension 3: Search Performance
     * All balanced structures are O(log n)
     * BST degrades for sorted input
     * B+ Tree has higher constant due to node traversal
     */
    double search_random_scores[DS_COUNT] = {8.0, 7.0, 9.0, 7.0};
    double search_sorted_scores[DS_COUNT] = {8.0, 7.0, 9.0, 1.0};

    for (int i = 0; i < DS_COUNT; i++) {
        double search_score = search_random_scores[i] * profile.data_randomness +
                              search_sorted_scores[i] * (1.0 - profile.data_randomness);
        scores[i] += search_score * profile.search_ratio * 2.0;
    }

    /*
     * Dimension 4: Dataset Size Suitability
     * Small datasets (<1000): BST is fine, simple and fast
     * Medium datasets: AVL or Skip List
     * Large datasets (>50K): B+ Tree shines (cache-friendly, low height)
     */
    if (profile.dataset_size < 1000) {
        scores[DS_BST] += 3.0;
        scores[DS_AVL] += 2.0;
    } else if (profile.dataset_size < 10000) {
        scores[DS_AVL] += 2.0;
        scores[DS_SKIPLIST] += 2.0;
    } else if (profile.dataset_size < 50000) {
        scores[DS_BPTREE] += 2.0;
        scores[DS_AVL] += 1.5;
    } else {
        scores[DS_BPTREE] += 4.0;
        scores[DS_SKIPLIST] += 1.0;
    }

    /*
     * Dimension 5: Sorted Data Penalty for BST
     */
    if (profile.data_randomness < 0.3) {
        scores[DS_BST] -= 5.0;  /* Heavy penalty for sorted data */
    }

    /* Find best */
    int best = 0;
    for (int i = 1; i < DS_COUNT; i++) {
        if (scores[i] > scores[best]) best = i;
    }

    /* Calculate confidence */
    double total = 0;
    for (int i = 0; i < DS_COUNT; i++) {
        if (scores[i] < 0) scores[i] = 0;
        total += scores[i];
    }

    result.selected = (DataStructureType)best;
    result.confidence = total > 0 ? scores[best] / total : 0.25;
    for (int i = 0; i < DS_COUNT; i++) {
        result.scores[i] = scores[i];
    }

    /* Generate explanation */
    switch (result.selected) {
        case DS_BPTREE:
            if (profile.range_ratio > 0.3)
                result.reason = "B+ Tree selected: High range query ratio benefits from linked leaf nodes.";
            else if (profile.dataset_size > 50000)
                result.reason = "B+ Tree selected: Large dataset benefits from high fan-out and low height.";
            else
                result.reason = "B+ Tree selected: Good all-around performance for this workload.";
            break;

        case DS_SKIPLIST:
            if (profile.insert_ratio > 0.4 && profile.data_randomness > 0.5)
                result.reason = "Skip List selected: High random insert ratio favors probabilistic insertion.";
            else
                result.reason = "Skip List selected: Good balance of insert/search without rotation overhead.";
            break;

        case DS_AVL:
            if (profile.search_ratio > 0.4)
                result.reason = "AVL Tree selected: Guaranteed O(log n) search with strict balancing.";
            else
                result.reason = "AVL Tree selected: Reliable balanced performance across all operations.";
            break;

        case DS_BST:
            if (profile.dataset_size < 1000)
                result.reason = "BST selected: Small dataset makes balancing overhead unnecessary.";
            else
                result.reason = "BST selected: Simple structure sufficient for this workload.";
            break;

        default:
            result.reason = "AVL Tree selected as safe default.";
            break;
    }

    return result;
}

/* ===================== Profile Printing ===================== */

void print_workload_profile(WorkloadProfile *profile) {
    printf("\n+------------------------------------------+\n");
    printf("|        WORKLOAD PROFILE                  |\n");
    printf("+------------------------------------------+\n");
    printf("|  Dataset Size:      %-20d|\n", profile->dataset_size);
    printf("|  Insert Ratio:      %-20.1f%%|\n", profile->insert_ratio * 100);
    printf("|  Search Ratio:      %-20.1f%%|\n", profile->search_ratio * 100);
    printf("|  Range Query Ratio: %-20.1f%%|\n", profile->range_ratio * 100);
    printf("|  Delete Ratio:      %-20.1f%%|\n", profile->delete_ratio * 100);
    printf("|  Data Randomness:   %-20.1f%%|\n", profile->data_randomness * 100);
    printf("|  Avg Range Span:    %-20d|\n", profile->range_query_size);
    printf("+------------------------------------------+\n\n");
}

void print_selection_result(SelectionResult *result) {
    printf("\n+------------------------------------------+\n");
    printf("|     ADAPTIVE SELECTION RESULT             |\n");
    printf("+------------------------------------------+\n");
    printf("|                                           |\n");
    printf("|  >>> %-35s|\n", ds_type_name(result->selected));
    printf("|  Confidence: %.0f%%                         |\n", result->confidence * 100);
    printf("|                                           |\n");
    printf("+------------------------------------------+\n");
    printf("|  Scores:                                  |\n");

    for (int i = 0; i < DS_COUNT; i++) {
        char bar[21];
        int filled = (int)(result->scores[i] / 2.0);
        if (filled > 20) filled = 20;
        if (filled < 0) filled = 0;
        for (int j = 0; j < 20; j++) {
            bar[j] = (j < filled) ? '#' : '.';
        }
        bar[20] = '\0';
        printf("|  %-8s %s %5.1f  |\n",
               ds_type_name((DataStructureType)i), bar, result->scores[i]);
    }

    printf("+------------------------------------------+\n");
    printf("|  Reason:                                  |\n");

    /* Word-wrap the reason string */
    const char *r = result->reason;
    while (*r) {
        printf("|  ");
        int count = 0;
        while (*r && count < 38) {
            putchar(*r);
            r++;
            count++;
        }
        while (count < 38) { putchar(' '); count++; }
        printf("  |\n");
    }

    printf("+------------------------------------------+\n\n");
}

/* ===================== Adaptive Wrapper ===================== */

AdaptiveDS* adaptive_create(DataStructureType type) {
    AdaptiveDS *ads = (AdaptiveDS *)malloc(sizeof(AdaptiveDS));
    if (!ads) return NULL;

    ads->type = type;
    ads->name = ds_type_name(type);

    switch (type) {
        case DS_BPTREE:
            ads->ds = bptree_create(BPTREE_DEFAULT_ORDER);
            break;
        case DS_SKIPLIST:
            ads->ds = skiplist_create();
            break;
        case DS_AVL:
            ads->ds = avl_create();
            break;
        case DS_BST:
            ads->ds = bst_create();
            break;
        default:
            free(ads);
            return NULL;
    }

    return ads;
}

void adaptive_insert(AdaptiveDS *ads, int key, const char *value) {
    if (!ads || !ads->ds) return;

    switch (ads->type) {
        case DS_BPTREE:  bptree_insert((BPTree *)ads->ds, key, value); break;
        case DS_SKIPLIST: skiplist_insert((SkipList *)ads->ds, key, value); break;
        case DS_AVL:     avl_insert((AVLTree *)ads->ds, key, value); break;
        case DS_BST:     bst_insert((BSTree *)ads->ds, key, value); break;
        default: break;
    }
}

char* adaptive_search(AdaptiveDS *ads, int key) {
    if (!ads || !ads->ds) return NULL;

    switch (ads->type) {
        case DS_BPTREE:  return bptree_search((BPTree *)ads->ds, key, NULL);
        case DS_SKIPLIST: return skiplist_search((SkipList *)ads->ds, key, NULL);
        case DS_AVL:     return avl_search((AVLTree *)ads->ds, key, NULL);
        case DS_BST:     return bst_search((BSTree *)ads->ds, key, NULL);
        default: return NULL;
    }
}

int adaptive_delete(AdaptiveDS *ads, int key) {
    if (!ads || !ads->ds) return 0;

    switch (ads->type) {
        case DS_BPTREE:  return bptree_delete((BPTree *)ads->ds, key);
        case DS_SKIPLIST: return skiplist_delete((SkipList *)ads->ds, key);
        case DS_AVL:     return avl_delete((AVLTree *)ads->ds, key);
        case DS_BST:     return bst_delete((BSTree *)ads->ds, key);
        default: return 0;
    }
}

void adaptive_print(AdaptiveDS *ads) {
    if (!ads || !ads->ds) return;

    printf("[Adaptive DS: %s]\n", ads->name);

    switch (ads->type) {
        case DS_BPTREE:  bptree_print((BPTree *)ads->ds); break;
        case DS_SKIPLIST: skiplist_print((SkipList *)ads->ds); break;
        case DS_AVL:     avl_print((AVLTree *)ads->ds); break;
        case DS_BST:     bst_print((BSTree *)ads->ds); break;
        default: break;
    }
}

void adaptive_destroy(AdaptiveDS *ads) {
    if (!ads) return;

    switch (ads->type) {
        case DS_BPTREE:  bptree_destroy((BPTree *)ads->ds); break;
        case DS_SKIPLIST: skiplist_destroy((SkipList *)ads->ds); break;
        case DS_AVL:     avl_destroy((AVLTree *)ads->ds); break;
        case DS_BST:     bst_destroy((BSTree *)ads->ds); break;
        default: break;
    }

    free(ads);
}

/* ===================== Predefined Workload Profiles ===================== */

WorkloadProfile workload_heavy_range(int size) {
    WorkloadProfile p = {
        .dataset_size = size,
        .insert_ratio = 0.15,
        .search_ratio = 0.15,
        .range_ratio = 0.60,
        .delete_ratio = 0.10,
        .data_randomness = 0.8,
        .range_query_size = size / 10
    };
    return p;
}

WorkloadProfile workload_heavy_insert(int size) {
    WorkloadProfile p = {
        .dataset_size = size,
        .insert_ratio = 0.60,
        .search_ratio = 0.20,
        .range_ratio = 0.10,
        .delete_ratio = 0.10,
        .data_randomness = 0.9,
        .range_query_size = 100
    };
    return p;
}

WorkloadProfile workload_heavy_search(int size) {
    WorkloadProfile p = {
        .dataset_size = size,
        .insert_ratio = 0.10,
        .search_ratio = 0.70,
        .range_ratio = 0.10,
        .delete_ratio = 0.10,
        .data_randomness = 0.7,
        .range_query_size = 100
    };
    return p;
}

WorkloadProfile workload_mixed(int size) {
    WorkloadProfile p = {
        .dataset_size = size,
        .insert_ratio = 0.30,
        .search_ratio = 0.30,
        .range_ratio = 0.20,
        .delete_ratio = 0.20,
        .data_randomness = 0.7,
        .range_query_size = size / 10
    };
    return p;
}

WorkloadProfile workload_small_dataset(void) {
    WorkloadProfile p = {
        .dataset_size = 500,
        .insert_ratio = 0.30,
        .search_ratio = 0.30,
        .range_ratio = 0.20,
        .delete_ratio = 0.20,
        .data_randomness = 0.8,
        .range_query_size = 50
    };
    return p;
}
