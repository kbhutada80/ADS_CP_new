#include "benchmark.h"
#include "avl.h"
#include "bst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * Benchmark Module Implementation
 *
 * Comprehensive performance comparison across all four data structures:
 *   1. B+ Tree    - Disk-optimized, linked leaves
 *   2. Skip List  - Probabilistic, no rotations
 *   3. AVL Tree   - Strictly balanced, guaranteed O(log n)
 *   4. BST        - Unbalanced baseline
 *
 * The module runs identical workloads on each structure and collects
 * timing data, traversal counts, and structural metrics.
 */

/* ===================== Utility ===================== */

static void _shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

static int* _generate_dataset(int size, int sorted) {
    int *data = (int *)malloc(size * sizeof(int));
    if (!data) return NULL;

    for (int i = 0; i < size; i++) {
        data[i] = i + 1;
    }

    if (!sorted) {
        _shuffle(data, size);
    }

    return data;
}

/* ===================== Individual Benchmarks ===================== */

void benchmark_bptree(BPTree *tree, int *dataset, int dataset_size, BenchmarkResult *result) {
    if (!tree || !dataset || !result) return;
    result->name = "B+ Tree";
    clock_t start, end;

    /* Insert */
    start = clock();
    for (int i = 0; i < dataset_size; i++) {
        char val[32];
        snprintf(val, sizeof(val), "v%d", dataset[i]);
        bptree_insert(tree, dataset[i], val);
    }
    end = clock();
    result->insert_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;

    /* Search */
    int total_trav = 0;
    start = clock();
    for (int i = 0; i < dataset_size; i++) {
        int dr = 0;
        bptree_search(tree, dataset[i], &dr);
        total_trav += dr;
    }
    end = clock();
    result->search_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    result->node_traversals = total_trav;

    /* Range Query: query 10% of the key range */
    int range_start = dataset_size / 4;
    int range_end = range_start + dataset_size / 10;
    if (range_end > dataset_size) range_end = dataset_size;

    start = clock();
    /* Walk linked leaves for range query */
    BPTreeLeafNode *leaf = tree->leftmost_leaf;
    int range_count = 0;
    while (leaf) {
        for (int i = 0; i < leaf->key_count; i++) {
            if (leaf->keys[i] >= range_start && leaf->keys[i] <= range_end) {
                range_count++;
            }
            if (leaf->keys[i] > range_end) goto bpt_range_done;
        }
        leaf = leaf->next;
    }
    bpt_range_done:
    end = clock();
    result->range_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    (void)range_count;

    result->height = bptree_height(tree);
    result->node_count = bptree_node_count(tree);
}

void benchmark_skiplist(SkipList *list, int *dataset, int dataset_size, BenchmarkResult *result) {
    if (!list || !dataset || !result) return;
    result->name = "Skip List";
    clock_t start, end;

    /* Insert */
    start = clock();
    for (int i = 0; i < dataset_size; i++) {
        char val[32];
        snprintf(val, sizeof(val), "v%d", dataset[i]);
        skiplist_insert(list, dataset[i], val);
    }
    end = clock();
    result->insert_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;

    /* Search */
    int total_trav = 0;
    start = clock();
    for (int i = 0; i < dataset_size; i++) {
        int acc = 0;
        skiplist_search(list, dataset[i], &acc);
        total_trav += acc;
    }
    end = clock();
    result->search_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    result->node_traversals = total_trav;

    /* Range Query: walk level-0 */
    int range_start = dataset_size / 4;
    int range_end = range_start + dataset_size / 10;

    start = clock();
    SkipListNode *node = list->header->forward[0];
    int range_count = 0;
    while (node) {
        if (node->key >= range_start && node->key <= range_end) {
            range_count++;
        }
        if (node->key > range_end) break;
        node = node->forward[0];
    }
    end = clock();
    result->range_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    (void)range_count;

    result->height = list->level;
    result->node_count = skiplist_node_count(list);
}

void benchmark_avl(AVLTree *tree, int *dataset, int dataset_size, BenchmarkResult *result) {
    if (!tree || !dataset || !result) return;
    result->name = "AVL Tree";
    clock_t start, end;

    /* Insert */
    start = clock();
    for (int i = 0; i < dataset_size; i++) {
        char val[32];
        snprintf(val, sizeof(val), "v%d", dataset[i]);
        avl_insert(tree, dataset[i], val);
    }
    end = clock();
    result->insert_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;

    /* Search */
    int total_trav = 0;
    start = clock();
    for (int i = 0; i < dataset_size; i++) {
        int trav = 0;
        avl_search(tree, dataset[i], &trav);
        total_trav += trav;
    }
    end = clock();
    result->search_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    result->node_traversals = total_trav;

    /* Range Query */
    int range_start = dataset_size / 4;
    int range_end = range_start + dataset_size / 10;
    int range_trav = 0;

    start = clock();
    AVLRangeResult *rr = avl_range_query(tree, range_start, range_end, &range_trav);
    end = clock();
    result->range_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    if (rr) avl_range_result_free(rr);

    result->height = avl_height(tree);
    result->node_count = avl_node_count(tree);
}

void benchmark_bst(BSTree *tree, int *dataset, int dataset_size, BenchmarkResult *result) {
    if (!tree || !dataset || !result) return;
    result->name = "BST";
    clock_t start, end;

    /* Insert */
    start = clock();
    for (int i = 0; i < dataset_size; i++) {
        char val[32];
        snprintf(val, sizeof(val), "v%d", dataset[i]);
        bst_insert(tree, dataset[i], val);
    }
    end = clock();
    result->insert_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;

    /* Search */
    int total_trav = 0;
    start = clock();
    for (int i = 0; i < dataset_size; i++) {
        int trav = 0;
        bst_search(tree, dataset[i], &trav);
        total_trav += trav;
    }
    end = clock();
    result->search_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    result->node_traversals = total_trav;

    /* Range Query */
    int range_start = dataset_size / 4;
    int range_end = range_start + dataset_size / 10;
    int range_trav = 0;

    start = clock();
    BSTRangeResult *rr = bst_range_query(tree, range_start, range_end, &range_trav);
    end = clock();
    result->range_time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    if (rr) bst_range_result_free(rr);

    result->height = bst_height(tree);
    result->node_count = bst_node_count(tree);
}

/* ===================== Full Benchmark ===================== */

ComparisonResult* run_full_benchmark(int dataset_size, int range_size) {
    printf("\n");
    printf("================================================================\n");
    printf("   BENCHMARK: All 4 Data Structures | N = %d\n", dataset_size);
    printf("================================================================\n");

    int *dataset = _generate_dataset(dataset_size, 0);
    if (!dataset) return NULL;

    ComparisonResult *cmp = (ComparisonResult *)calloc(1, sizeof(ComparisonResult));
    if (!cmp) { free(dataset); return NULL; }

    cmp->dataset_size = dataset_size;
    cmp->range_size = range_size;
    cmp->is_sorted = 0;

    /* B+ Tree */
    BPTree *bpt = bptree_create(4);
    benchmark_bptree(bpt, dataset, dataset_size, &cmp->results[0]);
    bptree_destroy(bpt);

    /* Skip List */
    SkipList *sl = skiplist_create();
    benchmark_skiplist(sl, dataset, dataset_size, &cmp->results[1]);
    skiplist_destroy(sl);

    /* AVL Tree */
    AVLTree *avl = avl_create();
    benchmark_avl(avl, dataset, dataset_size, &cmp->results[2]);
    avl_destroy(avl);

    /* BST */
    BSTree *bst = bst_create();
    benchmark_bst(bst, dataset, dataset_size, &cmp->results[3]);
    bst_destroy(bst);

    free(dataset);
    return cmp;
}

MultiBenchmarkResult* run_multi_benchmark(int *sizes, int num_sizes, int sorted) {
    MultiBenchmarkResult *mbr = (MultiBenchmarkResult *)malloc(sizeof(MultiBenchmarkResult));
    if (!mbr) return NULL;

    mbr->num_sizes = num_sizes;
    mbr->is_sorted = sorted;
    mbr->sizes = (int *)malloc(num_sizes * sizeof(int));
    mbr->comparisons = (ComparisonResult *)calloc(num_sizes, sizeof(ComparisonResult));

    if (!mbr->sizes || !mbr->comparisons) {
        free(mbr->sizes);
        free(mbr->comparisons);
        free(mbr);
        return NULL;
    }

    memcpy(mbr->sizes, sizes, num_sizes * sizeof(int));

    printf("\n");
    printf("+---------------------------------------------------------------+\n");
    printf("|         MULTI-SIZE BENCHMARK (%s input)               |\n",
           sorted ? "SORTED" : "RANDOM");
    printf("|         Testing sizes:");

    for (int i = 0; i < num_sizes; i++) {
        printf(" %d", sizes[i]);
    }
    printf("\n");
    printf("+---------------------------------------------------------------+\n");

    for (int s = 0; s < num_sizes; s++) {
        int *dataset = _generate_dataset(sizes[s], sorted);
        if (!dataset) continue;

        ComparisonResult *cmp = &mbr->comparisons[s];
        cmp->dataset_size = sizes[s];
        cmp->range_size = sizes[s] / 10;
        cmp->is_sorted = sorted;

        printf("\n--- Dataset Size: %d ---\n", sizes[s]);

        BPTree *bpt = bptree_create(4);
        benchmark_bptree(bpt, dataset, sizes[s], &cmp->results[0]);
        bptree_destroy(bpt);

        SkipList *sl = skiplist_create();
        benchmark_skiplist(sl, dataset, sizes[s], &cmp->results[1]);
        skiplist_destroy(sl);

        AVLTree *avl = avl_create();
        benchmark_avl(avl, dataset, sizes[s], &cmp->results[2]);
        avl_destroy(avl);

        BSTree *bst = bst_create();
        benchmark_bst(bst, dataset, sizes[s], &cmp->results[3]);
        bst_destroy(bst);

        free(dataset);
    }

    return mbr;
}

/* ===================== Output Functions ===================== */

void print_benchmark_results(ComparisonResult *results) {
    if (!results) return;

    printf("\n================================================================\n");
    printf("   DETAILED RESULTS | N = %d\n", results->dataset_size);
    printf("================================================================\n\n");

    for (int i = 0; i < NUM_STRUCTURES; i++) {
        BenchmarkResult *r = &results->results[i];
        printf("  %s:\n", r->name);
        printf("    Insert Time:      %8.3f ms\n", r->insert_time_ms);
        printf("    Search Time:      %8.3f ms\n", r->search_time_ms);
        printf("    Range Query Time: %8.3f ms\n", r->range_time_ms);
        printf("    Node Traversals:  %8d\n", r->node_traversals);
        printf("    Height:           %8d\n", r->height);
        printf("\n");
    }
}

void print_comparison_table(ComparisonResult *results) {
    if (!results) return;

    printf("\n+-------------------------------------------------------------------------------+\n");
    printf("|               PERFORMANCE COMPARISON TABLE  |  N = %-6d                    |\n",
           results->dataset_size);
    printf("+-------------------------------------------------------------------------------+\n");

    printf("| %-18s | %10s | %10s | %10s | %10s | %-9s|\n",
           "Metric", "B+ Tree", "Skip List", "AVL Tree", "BST", "Winner");
    printf("|------------------+------------+------------+------------+------------+----------|\n");

    /* Insert */
    int best_ins = 0;
    for (int i = 1; i < NUM_STRUCTURES; i++)
        if (results->results[i].insert_time_ms < results->results[best_ins].insert_time_ms) best_ins = i;
    printf("| %-18s| %8.3f ms| %8.3f ms| %8.3f ms| %8.3f ms| %-9s|\n",
           "Insert Time",
           results->results[0].insert_time_ms, results->results[1].insert_time_ms,
           results->results[2].insert_time_ms, results->results[3].insert_time_ms,
           results->results[best_ins].name);

    /* Search */
    int best_sch = 0;
    for (int i = 1; i < NUM_STRUCTURES; i++)
        if (results->results[i].search_time_ms < results->results[best_sch].search_time_ms) best_sch = i;
    printf("| %-18s| %8.3f ms| %8.3f ms| %8.3f ms| %8.3f ms| %-9s|\n",
           "Search Time",
           results->results[0].search_time_ms, results->results[1].search_time_ms,
           results->results[2].search_time_ms, results->results[3].search_time_ms,
           results->results[best_sch].name);

    /* Range Query */
    int best_rng = 0;
    for (int i = 1; i < NUM_STRUCTURES; i++)
        if (results->results[i].range_time_ms < results->results[best_rng].range_time_ms) best_rng = i;
    printf("| %-18s| %8.3f ms| %8.3f ms| %8.3f ms| %8.3f ms| %-9s|\n",
           "Range Query",
           results->results[0].range_time_ms, results->results[1].range_time_ms,
           results->results[2].range_time_ms, results->results[3].range_time_ms,
           results->results[best_rng].name);

    /* Traversals */
    int best_trv = 0;
    for (int i = 1; i < NUM_STRUCTURES; i++)
        if (results->results[i].node_traversals < results->results[best_trv].node_traversals) best_trv = i;
    printf("| %-18s| %10d| %10d| %10d| %10d| %-9s|\n",
           "Node Traversals",
           results->results[0].node_traversals, results->results[1].node_traversals,
           results->results[2].node_traversals, results->results[3].node_traversals,
           results->results[best_trv].name);

    /* Height */
    int best_h = 0;
    for (int i = 1; i < NUM_STRUCTURES; i++)
        if (results->results[i].height < results->results[best_h].height) best_h = i;
    printf("| %-18s| %10d| %10d| %10d| %10d| %-9s|\n",
           "Height",
           results->results[0].height, results->results[1].height,
           results->results[2].height, results->results[3].height,
           results->results[best_h].name);

    printf("+-------------------------------------------------------------------------------+\n\n");
}

void print_multi_results(MultiBenchmarkResult *mbr) {
    if (!mbr) return;

    for (int s = 0; s < mbr->num_sizes; s++) {
        print_comparison_table(&mbr->comparisons[s]);
    }

    /* Summary table across all sizes */
    printf("\n+-------------------------------------------------------------------------------+\n");
    printf("|                    SUMMARY ACROSS ALL DATASET SIZES                          |\n");
    printf("+-------------------------------------------------------------------------------+\n\n");

    printf("  %-8s |", "N");
    for (int i = 0; i < NUM_STRUCTURES; i++) {
        printf(" %-10s |", mbr->comparisons[0].results[i].name);
    }
    printf("\n");

    printf("  ---------+");
    for (int i = 0; i < NUM_STRUCTURES; i++) {
        printf("------------+");
        (void)i;
    }
    printf("\n");

    printf("  INSERT TIME (ms):\n");
    for (int s = 0; s < mbr->num_sizes; s++) {
        printf("  %-8d |", mbr->sizes[s]);
        for (int i = 0; i < NUM_STRUCTURES; i++) {
            printf(" %8.3f   |", mbr->comparisons[s].results[i].insert_time_ms);
        }
        printf("\n");
    }

    printf("\n  SEARCH TIME (ms):\n");
    for (int s = 0; s < mbr->num_sizes; s++) {
        printf("  %-8d |", mbr->sizes[s]);
        for (int i = 0; i < NUM_STRUCTURES; i++) {
            printf(" %8.3f   |", mbr->comparisons[s].results[i].search_time_ms);
        }
        printf("\n");
    }

    printf("\n  RANGE QUERY TIME (ms):\n");
    for (int s = 0; s < mbr->num_sizes; s++) {
        printf("  %-8d |", mbr->sizes[s]);
        for (int i = 0; i < NUM_STRUCTURES; i++) {
            printf(" %8.3f   |", mbr->comparisons[s].results[i].range_time_ms);
        }
        printf("\n");
    }

    printf("\n  HEIGHT:\n");
    for (int s = 0; s < mbr->num_sizes; s++) {
        printf("  %-8d |", mbr->sizes[s]);
        for (int i = 0; i < NUM_STRUCTURES; i++) {
            printf(" %8d   |", mbr->comparisons[s].results[i].height);
        }
        printf("\n");
    }

    printf("\n+-------------------------------------------------------------------------------+\n");
}

/* ===================== CSV Export ===================== */

void export_csv(MultiBenchmarkResult *mbr, const char *filename) {
    if (!mbr || !filename) return;

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot create CSV file '%s'\n", filename);
        return;
    }

    /* Header */
    fprintf(fp, "DataSize,Metric,BPlusTree,SkipList,AVLTree,BST\n");

    for (int s = 0; s < mbr->num_sizes; s++) {
        ComparisonResult *c = &mbr->comparisons[s];
        int n = mbr->sizes[s];

        fprintf(fp, "%d,InsertTime_ms,%.4f,%.4f,%.4f,%.4f\n", n,
                c->results[0].insert_time_ms, c->results[1].insert_time_ms,
                c->results[2].insert_time_ms, c->results[3].insert_time_ms);

        fprintf(fp, "%d,SearchTime_ms,%.4f,%.4f,%.4f,%.4f\n", n,
                c->results[0].search_time_ms, c->results[1].search_time_ms,
                c->results[2].search_time_ms, c->results[3].search_time_ms);

        fprintf(fp, "%d,RangeTime_ms,%.4f,%.4f,%.4f,%.4f\n", n,
                c->results[0].range_time_ms, c->results[1].range_time_ms,
                c->results[2].range_time_ms, c->results[3].range_time_ms);

        fprintf(fp, "%d,Height,%d,%d,%d,%d\n", n,
                c->results[0].height, c->results[1].height,
                c->results[2].height, c->results[3].height);

        fprintf(fp, "%d,Traversals,%d,%d,%d,%d\n", n,
                c->results[0].node_traversals, c->results[1].node_traversals,
                c->results[2].node_traversals, c->results[3].node_traversals);
    }

    fclose(fp);
    printf("\n[CSV] Benchmark results exported to: %s\n", filename);
}

/* ===================== Observation Engine ===================== */

void print_observations(MultiBenchmarkResult *mbr) {
    if (!mbr || mbr->num_sizes < 2) return;

    printf("\n");
    printf("+---------------------------------------------------------------+\n");
    printf("|                OBSERVATION ENGINE                            |\n");
    printf("|        Automated Insights from Benchmark Results             |\n");
    printf("+---------------------------------------------------------------+\n\n");

    int last = mbr->num_sizes - 1;
    ComparisonResult *small = &mbr->comparisons[0];
    ComparisonResult *large = &mbr->comparisons[last];

    int obs_count = 0;

    /* Observation 1: Range Query Analysis */
    int best_range = 0;
    for (int i = 1; i < NUM_STRUCTURES; i++) {
        if (large->results[i].range_time_ms < large->results[best_range].range_time_ms)
            best_range = i;
    }

    if (best_range == 0) {
        printf("  [%d] B+ Tree performs best for range queries (%.3f ms at N=%d).\n",
               ++obs_count, large->results[0].range_time_ms, large->dataset_size);
        printf("       >> This is due to linked leaf nodes enabling sequential scan\n");
        printf("          without tree traversal overhead.\n\n");
    } else {
        printf("  [%d] %s performs best for range queries (%.3f ms at N=%d).\n",
               ++obs_count, large->results[best_range].name,
               large->results[best_range].range_time_ms, large->dataset_size);
        printf("       >> B+ Tree range query: %.3f ms for comparison.\n\n",
               large->results[0].range_time_ms);
    }

    /* Observation 2: Insert Performance */
    int best_insert = 0;
    for (int i = 1; i < NUM_STRUCTURES; i++) {
        if (large->results[i].insert_time_ms < large->results[best_insert].insert_time_ms)
            best_insert = i;
    }

    if (best_insert == 1) {
        printf("  [%d] Skip List shows faster insertion for randomized data (%.3f ms).\n",
               ++obs_count, large->results[1].insert_time_ms);
        printf("       >> Probabilistic level generation avoids rotation overhead.\n\n");
    } else {
        printf("  [%d] %s has the fastest insertion (%.3f ms at N=%d).\n",
               ++obs_count, large->results[best_insert].name,
               large->results[best_insert].insert_time_ms, large->dataset_size);
        printf("\n");
    }

    /* Observation 3: BST Degradation for Sorted Input */
    if (mbr->is_sorted) {
        printf("  [%d] BST degrades severely for sorted input!\n", ++obs_count);
        printf("       >> Height at N=%d: BST=%d vs AVL=%d vs B+Tree=%d\n",
               large->dataset_size, large->results[3].height,
               large->results[2].height, large->results[0].height);
        printf("       >> BST becomes a linked list (height = N), all ops become O(n).\n\n");
    } else if (large->results[3].height > large->results[2].height * 2) {
        printf("  [%d] BST height (%d) is significantly higher than AVL (%d) at N=%d.\n",
               ++obs_count, large->results[3].height, large->results[2].height,
               large->dataset_size);
        printf("       >> Even with random input, BST lacks balance guarantees.\n\n");
    }

    /* Observation 4: AVL Balancing Overhead */
    if (large->results[2].insert_time_ms > large->results[3].insert_time_ms * 1.1) {
        printf("  [%d] AVL Tree maintains balanced height (%d) but has %.1f%% higher\n",
               ++obs_count, large->results[2].height,
               (large->results[2].insert_time_ms / large->results[3].insert_time_ms - 1.0) * 100);
        printf("       insertion overhead compared to BST due to rotation operations.\n\n");
    } else {
        printf("  [%d] AVL Tree maintains balanced height (%d) with competitive\n",
               ++obs_count, large->results[2].height);
        printf("       insertion performance compared to unbalanced BST.\n\n");
    }

    /* Observation 5: Height Comparison */
    printf("  [%d] Height comparison at N=%d:\n", ++obs_count, large->dataset_size);
    printf("       B+ Tree: %d | Skip List: %d | AVL: %d | BST: %d\n",
           large->results[0].height, large->results[1].height,
           large->results[2].height, large->results[3].height);
    printf("       >> B+ Tree achieves lowest height due to high fan-out (order).\n");
    printf("       >> AVL guarantees O(log n) height via strict balancing.\n\n");

    /* Observation 6: Search Efficiency */
    int best_search = 0;
    for (int i = 1; i < NUM_STRUCTURES; i++) {
        if (large->results[i].search_time_ms < large->results[best_search].search_time_ms)
            best_search = i;
    }
    printf("  [%d] Most efficient search: %s (%.3f ms for %d queries).\n",
           ++obs_count, large->results[best_search].name,
           large->results[best_search].search_time_ms, large->dataset_size);
    printf("       >> Node traversals: B+Tree=%d, Skip=%d, AVL=%d, BST=%d\n\n",
           large->results[0].node_traversals, large->results[1].node_traversals,
           large->results[2].node_traversals, large->results[3].node_traversals);

    /* Observation 7: Scaling Behavior */
    double bpt_scale = (small->results[0].insert_time_ms > 0) ?
        large->results[0].insert_time_ms / small->results[0].insert_time_ms : 0;
    double bst_scale = (small->results[3].insert_time_ms > 0) ?
        large->results[3].insert_time_ms / small->results[3].insert_time_ms : 0;

    printf("  [%d] Scaling behavior (insert time ratio, N=%d to N=%d):\n",
           ++obs_count, small->dataset_size, large->dataset_size);
    printf("       B+ Tree: %.1fx | BST: %.1fx\n", bpt_scale, bst_scale);
    if (bst_scale > bpt_scale * 2) {
        printf("       >> BST scales much worse, confirming O(n) degradation risk.\n\n");
    } else {
        printf("       >> Both scale comparably with random input.\n\n");
    }

    printf("  ------------------------------------------------------------\n");
    printf("  Total observations generated: %d\n\n", obs_count);
}

/* ===================== Theoretical Analysis ===================== */

void print_theoretical_analysis(void) {
    printf("\n");
    printf("+-----------------------------------------------------------------------------------+\n");
    printf("|                        THEORETICAL COMPLEXITY ANALYSIS                            |\n");
    printf("+-----------------------------------------------------------------------------------+\n\n");

    printf("  +--------------+----------------+----------------+----------------+----------+\n");
    printf("  | Operation    | B+ Tree        | Skip List      | AVL Tree       | BST      |\n");
    printf("  +--------------+----------------+----------------+----------------+----------+\n");
    printf("  | Insert Best  | O(log n)       | O(log n)       | O(log n)       | O(log n) |\n");
    printf("  | Insert Avg   | O(log n)       | O(log n)       | O(log n)       | O(log n) |\n");
    printf("  | Insert Worst | O(log n)       | O(n)*          | O(log n)       | O(n)     |\n");
    printf("  +--------------+----------------+----------------+----------------+----------+\n");
    printf("  | Search Best  | O(log n)       | O(log n)       | O(log n)       | O(1)     |\n");
    printf("  | Search Avg   | O(log n)       | O(log n)       | O(log n)       | O(log n) |\n");
    printf("  | Search Worst | O(log n)       | O(n)*          | O(log n)       | O(n)     |\n");
    printf("  +--------------+----------------+----------------+----------------+----------+\n");
    printf("  | Delete Best  | O(log n)       | O(log n)       | O(log n)       | O(log n) |\n");
    printf("  | Delete Avg   | O(log n)       | O(log n)       | O(log n)       | O(log n) |\n");
    printf("  | Delete Worst | O(log n)       | O(n)*          | O(log n)       | O(n)     |\n");
    printf("  +--------------+----------------+----------------+----------------+----------+\n");
    printf("  | Range Query  | O(log n + k)   | O(log n + k)   | O(log n + k)   | O(n + k) |\n");
    printf("  +--------------+----------------+----------------+----------------+----------+\n");
    printf("  | Space        | O(n)           | O(n log n)     | O(n)           | O(n)     |\n");
    printf("  | Height       | O(log_m n)     | O(log n) exp   | O(log n)       | O(n)     |\n");
    printf("  +--------------+----------------+----------------+----------------+----------+\n");
    printf("\n");
    printf("  * Skip List worst case is O(n) but occurs with probability ~(1/2)^n\n");
    printf("  m = order/fan-out of B+ Tree; k = number of results in range query\n");
    printf("\n");

    printf("  BEHAVIOR UNDER SORTED INPUT:\n");
    printf("  ------------------------------\n");
    printf("  B+ Tree:   Handles sorted input well. Height stays O(log_m n).\n");
    printf("             Splits distribute keys evenly across leaves.\n\n");
    printf("  Skip List: Performance is independent of input order.\n");
    printf("             Randomized levels ensure expected O(log n) height.\n\n");
    printf("  AVL Tree:  Handles sorted input well. Rotations keep height <= 1.44*log(n).\n");
    printf("             May perform many rotations during insertion.\n\n");
    printf("  BST:       DEGRADES to O(n) for sorted input! Tree becomes a linked list.\n");
    printf("             Height = n, all operations become O(n).\n\n");

    printf("  BEHAVIOR UNDER RANDOM INPUT:\n");
    printf("  ------------------------------\n");
    printf("  B+ Tree:   Excellent performance. Balanced by construction.\n");
    printf("  Skip List: Expected O(log n) for all operations.\n");
    printf("  AVL Tree:  O(log n) guaranteed. Slight overhead from rotations.\n");
    printf("  BST:       Expected height ~1.39*log(n). Generally good but not guaranteed.\n\n");

    printf("+-----------------------------------------------------------------------------------+\n\n");
}

/* ===================== Cleanup ===================== */

void benchmark_cleanup(ComparisonResult *results) {
    free(results);
}

void multi_benchmark_cleanup(MultiBenchmarkResult *mbr) {
    if (!mbr) return;
    free(mbr->sizes);
    free(mbr->comparisons);
    free(mbr);
}
