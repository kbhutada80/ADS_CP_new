#include "bptree.h"
#include "skiplist.h"
#include "avl.h"
#include "bst.h"
#include "benchmark.h"
#include "adaptive.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Comparative Analysis and Adaptive Selection of Data Structures
 * for Efficient Range Queries
 *
 * Main CLI Interface
 *
 * Supported commands:
 *   insert <key> <value>     - Insert key-value pair into active structure
 *   search <key>             - Search for a key
 *   delete <key>             - Delete a key
 *   range <start> <end>      - Range query [start, end]
 *   benchmark                - Run full benchmark (all sizes, random input)
 *   benchmark_sorted         - Run benchmark with sorted input
 *   auto                     - Adaptive selection demo
 *   theory                   - Print theoretical analysis
 *   print_structure           - Print current data structure
 *   switch <bptree|skiplist|avl|bst>  - Switch active data structure
 *   help                     - Show help
 *   exit                     - Exit program
 */

/* ===================== Globals ===================== */

static AdaptiveDS *active_ds = NULL;
static DataStructureType active_type = DS_AVL;

/* ===================== Help ===================== */

static void print_banner(void) {
    printf("\n");
    printf("+---------------------------------------------------------------+\n");
    printf("|   Comparative Analysis & Adaptive Selection of              |\n");
    printf("|   Data Structures for Efficient Range Queries               |\n");
    printf("+---------------------------------------------------------------+\n");
    printf("|   Data Structures: B+ Tree | Skip List | AVL Tree | BST    |\n");
    printf("|   Type 'help' for available commands                        |\n");
    printf("+---------------------------------------------------------------+\n\n");
}

static void print_help(void) {
    printf("\n");
    printf("+---------------------------------------------------------------+\n");
    printf("|                    AVAILABLE COMMANDS                        |\n");
    printf("+---------------------------------------------------------------+\n");
    printf("|                                                              |\n");
    printf("|  DATA OPERATIONS:                                            |\n");
    printf("|    insert <key> <value>   Insert key-value pair              |\n");
    printf("|    search <key>           Search for a key                   |\n");
    printf("|    delete <key>           Delete a key                       |\n");
    printf("|    range <start> <end>    Range query (inclusive)             |\n");
    printf("|                                                              |\n");
    printf("|  BENCHMARKING:                                               |\n");
    printf("|    benchmark              Full benchmark (random input)       |\n");
    printf("|    benchmark_sorted       Full benchmark (sorted input)       |\n");
    printf("|    benchmark_quick        Quick benchmark (small sizes)       |\n");
    printf("|    theory                 Theoretical complexity analysis     |\n");
    printf("|                                                              |\n");
    printf("|  ADAPTIVE SYSTEM:                                            |\n");
    printf("|    auto                   Run adaptive selection demo         |\n");
    printf("|    auto_custom            Custom workload profile             |\n");
    printf("|                                                              |\n");
    printf("|  STRUCTURE MANAGEMENT:                                       |\n");
    printf("|    switch <type>          Switch active DS                    |\n");
    printf("|         types: bptree, skiplist, avl, bst                    |\n");
    printf("|    print_structure        Print current DS contents           |\n");
    printf("|    load_sample            Load sample data                    |\n");
    printf("|                                                              |\n");
    printf("|  OTHER:                                                      |\n");
    printf("|    help                   Show this help                      |\n");
    printf("|    exit / quit            Exit program                        |\n");
    printf("|                                                              |\n");
    printf("+---------------------------------------------------------------+\n\n");
}

/* ===================== DS Switching ===================== */

static void switch_ds(const char *type_str) {
    DataStructureType new_type;

    if (strcmp(type_str, "bptree") == 0) {
        new_type = DS_BPTREE;
    } else if (strcmp(type_str, "skiplist") == 0) {
        new_type = DS_SKIPLIST;
    } else if (strcmp(type_str, "avl") == 0) {
        new_type = DS_AVL;
    } else if (strcmp(type_str, "bst") == 0) {
        new_type = DS_BST;
    } else {
        printf("Unknown type: %s\n", type_str);
        printf("Valid types: bptree, skiplist, avl, bst\n");
        return;
    }

    if (active_ds) {
        adaptive_destroy(active_ds);
    }

    active_type = new_type;
    active_ds = adaptive_create(new_type);
    printf("Switched to: %s\n", ds_type_name(new_type));
}

/* ===================== Sample Data ===================== */

static void load_sample_data(void) {
    printf("Loading sample data into %s...\n", ds_type_name(active_type));

    struct { int key; const char *val; } samples[] = {
        {10, "apple"},    {20, "banana"},    {30, "cherry"},
        {40, "date"},     {50, "elderberry"},{60, "fig"},
        {70, "grape"},    {80, "honeydew"},  {90, "iris"},
        {100, "jackfruit"},{5, "avocado"},   {15, "blueberry"},
        {25, "coconut"},  {35, "dragonfruit"},{45, "eggplant"},
        {55, "fennel"},   {65, "guava"},     {75, "hazelnut"},
        {85, "jalapeno"}, {95, "kiwi"}
    };

    int count = sizeof(samples) / sizeof(samples[0]);
    for (int i = 0; i < count; i++) {
        adaptive_insert(active_ds, samples[i].key, samples[i].val);
    }
    printf("Loaded %d sample items.\n", count);
}

/* ===================== Range Query Dispatch ===================== */

static void do_range_query(int start, int end) {
    printf("Range query [%d, %d] on %s:\n\n", start, end, ds_type_name(active_type));

    switch (active_type) {
        case DS_BPTREE: {
            BPTree *tree = (BPTree *)active_ds->ds;
            BPTreeLeafNode *leaf = tree->leftmost_leaf;
            int count = 0;
            while (leaf) {
                for (int i = 0; i < leaf->key_count; i++) {
                    if (leaf->keys[i] >= start && leaf->keys[i] <= end) {
                        printf("  %d => %s\n", leaf->keys[i], leaf->values[i]);
                        count++;
                    }
                    if (leaf->keys[i] > end) goto bpt_done;
                }
                leaf = leaf->next;
            }
            bpt_done:
            printf("\nFound %d results.\n", count);
            break;
        }
        case DS_SKIPLIST: {
            SkipList *sl = (SkipList *)active_ds->ds;
            SkipListNode *node = sl->header->forward[0];
            int count = 0;
            while (node) {
                if (node->key >= start && node->key <= end) {
                    printf("  %d => %s\n", node->key, node->value);
                    count++;
                }
                if (node->key > end) break;
                node = node->forward[0];
            }
            printf("\nFound %d results.\n", count);
            break;
        }
        case DS_AVL: {
            int trav = 0;
            AVLRangeResult *rr = avl_range_query((AVLTree *)active_ds->ds, start, end, &trav);
            if (rr) {
                for (int i = 0; i < rr->count; i++) {
                    printf("  %d => %s\n", rr->keys[i], rr->values[i]);
                }
                printf("\nFound %d results (traversals: %d).\n", rr->count, trav);
                avl_range_result_free(rr);
            }
            break;
        }
        case DS_BST: {
            int trav = 0;
            BSTRangeResult *rr = bst_range_query((BSTree *)active_ds->ds, start, end, &trav);
            if (rr) {
                for (int i = 0; i < rr->count; i++) {
                    printf("  %d => %s\n", rr->keys[i], rr->values[i]);
                }
                printf("\nFound %d results (traversals: %d).\n", rr->count, trav);
                bst_range_result_free(rr);
            }
            break;
        }
        default:
            printf("Range query not supported for this structure.\n");
    }
}

/* ===================== Adaptive Demo ===================== */

static void run_adaptive_demo(void) {
    printf("\n");
    printf("+---------------------------------------------------------------+\n");
    printf("|              ADAPTIVE SELECTION SYSTEM DEMO                  |\n");
    printf("+---------------------------------------------------------------+\n\n");

    /* Test multiple workload profiles */
    struct { const char *name; WorkloadProfile profile; } workloads[] = {
        {"Heavy Range Queries (N=10000)",   workload_heavy_range(10000)},
        {"Heavy Inserts - Random (N=10000)",workload_heavy_insert(10000)},
        {"Heavy Searches (N=10000)",        workload_heavy_search(10000)},
        {"Mixed Workload (N=10000)",        workload_mixed(10000)},
        {"Small Dataset (N=500)",           workload_small_dataset()},
        {"Large Range Queries (N=100000)",  workload_heavy_range(100000)},
    };

    int n = sizeof(workloads) / sizeof(workloads[0]);

    for (int i = 0; i < n; i++) {
        printf("--- Workload: %s ---\n", workloads[i].name);
        print_workload_profile(&workloads[i].profile);

        SelectionResult result = select_data_structure(workloads[i].profile);
        print_selection_result(&result);

        /* Demonstrate using the selected structure */
        AdaptiveDS *ads = adaptive_create(result.selected);
        printf("  Demo: Inserting 100 items into %s...\n", ds_type_name(result.selected));
        for (int j = 1; j <= 100; j++) {
            char val[16];
            snprintf(val, sizeof(val), "item_%d", j);
            adaptive_insert(ads, j, val);
        }
        char *found = adaptive_search(ads, 50);
        printf("  Demo: Search(50) = %s\n", found ? found : "NOT FOUND");
        adaptive_destroy(ads);
        printf("\n");
    }
}

static void run_adaptive_custom(void) {
    WorkloadProfile p;
    printf("\n--- Custom Workload Profile ---\n");
    printf("Enter dataset size: ");
    if (scanf("%d", &p.dataset_size) != 1) { while(getchar() != '\n'); return; }
    printf("Insert ratio (0.0-1.0): ");
    if (scanf("%lf", &p.insert_ratio) != 1) { while(getchar() != '\n'); return; }
    printf("Search ratio (0.0-1.0): ");
    if (scanf("%lf", &p.search_ratio) != 1) { while(getchar() != '\n'); return; }
    printf("Range query ratio (0.0-1.0): ");
    if (scanf("%lf", &p.range_ratio) != 1) { while(getchar() != '\n'); return; }
    p.delete_ratio = 1.0 - p.insert_ratio - p.search_ratio - p.range_ratio;
    if (p.delete_ratio < 0) p.delete_ratio = 0;
    printf("Data randomness (0.0=sorted, 1.0=random): ");
    if (scanf("%lf", &p.data_randomness) != 1) { while(getchar() != '\n'); return; }
    p.range_query_size = p.dataset_size / 10;

    /* Consume trailing newline */
    while(getchar() != '\n');

    print_workload_profile(&p);
    SelectionResult result = select_data_structure(p);
    print_selection_result(&result);
}

/* ===================== Command Execution ===================== */

static void execute_command(const char *command) {
    if (!command || strlen(command) == 0) return;

    char cmd[64];
    int key, start_key, end_key;
    char value[256];
    char type_str[32];

    if (sscanf(command, "%63s", cmd) < 1) return;

    if (strcmp(cmd, "insert") == 0) {
        if (sscanf(command, "%*s %d %255s", &key, value) == 2) {
            adaptive_insert(active_ds, key, value);
            printf("[%s] Inserted: %d => %s\n", ds_type_name(active_type), key, value);
        } else {
            printf("Usage: insert <key> <value>\n");
        }

    } else if (strcmp(cmd, "search") == 0) {
        if (sscanf(command, "%*s %d", &key) == 1) {
            char *result = adaptive_search(active_ds, key);
            if (result)
                printf("[%s] Found: %d => %s\n", ds_type_name(active_type), key, result);
            else
                printf("[%s] Key %d not found.\n", ds_type_name(active_type), key);
        } else {
            printf("Usage: search <key>\n");
        }

    } else if (strcmp(cmd, "delete") == 0) {
        if (sscanf(command, "%*s %d", &key) == 1) {
            int ok = adaptive_delete(active_ds, key);
            if (ok)
                printf("[%s] Deleted key %d.\n", ds_type_name(active_type), key);
            else
                printf("[%s] Key %d not found.\n", ds_type_name(active_type), key);
        } else {
            printf("Usage: delete <key>\n");
        }

    } else if (strcmp(cmd, "range") == 0) {
        if (sscanf(command, "%*s %d %d", &start_key, &end_key) == 2) {
            do_range_query(start_key, end_key);
        } else {
            printf("Usage: range <start> <end>\n");
        }

    } else if (strcmp(cmd, "benchmark") == 0) {
        int sizes[] = {1000, 5000, 10000, 50000, 100000};
        int n = 5;
        MultiBenchmarkResult *mbr = run_multi_benchmark(sizes, n, 0);
        if (mbr) {
            print_multi_results(mbr);
            export_csv(mbr, "benchmark_random.csv");
            print_observations(mbr);
            multi_benchmark_cleanup(mbr);
        }

    } else if (strcmp(cmd, "benchmark_sorted") == 0) {
        int sizes[] = {1000, 5000, 10000, 50000};
        int n = 4;
        MultiBenchmarkResult *mbr = run_multi_benchmark(sizes, n, 1);
        if (mbr) {
            print_multi_results(mbr);
            export_csv(mbr, "benchmark_sorted.csv");
            print_observations(mbr);
            multi_benchmark_cleanup(mbr);
        }

    } else if (strcmp(cmd, "benchmark_quick") == 0) {
        int sizes[] = {1000, 5000, 10000};
        int n = 3;
        MultiBenchmarkResult *mbr = run_multi_benchmark(sizes, n, 0);
        if (mbr) {
            print_multi_results(mbr);
            export_csv(mbr, "benchmark_quick.csv");
            print_observations(mbr);
            multi_benchmark_cleanup(mbr);
        }

    } else if (strcmp(cmd, "auto") == 0) {
        run_adaptive_demo();

    } else if (strcmp(cmd, "auto_custom") == 0) {
        run_adaptive_custom();

    } else if (strcmp(cmd, "theory") == 0) {
        print_theoretical_analysis();

    } else if (strcmp(cmd, "switch") == 0) {
        if (sscanf(command, "%*s %31s", type_str) == 1) {
            switch_ds(type_str);
        } else {
            printf("Usage: switch <bptree|skiplist|avl|bst>\n");
        }

    } else if (strcmp(cmd, "print_structure") == 0) {
        adaptive_print(active_ds);

    } else if (strcmp(cmd, "load_sample") == 0) {
        load_sample_data();

    } else if (strcmp(cmd, "help") == 0) {
        print_help();

    } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        /* Handled in loop */
        return;

    } else {
        printf("Unknown command: '%s'. Type 'help' for available commands.\n", cmd);
    }
}

/* ===================== Interactive Loop ===================== */

static void interactive_mode(void) {
    char command[1024];

    print_banner();

    /* Initialize default data structure */
    active_ds = adaptive_create(active_type);
    printf("Active data structure: %s\n", ds_type_name(active_type));
    printf("Type 'help' for available commands.\n\n");

    while (1) {
        printf("ds> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL) break;
        command[strcspn(command, "\n")] = '\0';

        if (strlen(command) == 0) continue;

        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            printf("Goodbye!\n");
            break;
        }

        execute_command(command);
    }

    if (active_ds) {
        adaptive_destroy(active_ds);
        active_ds = NULL;
    }
}

/* ===================== Interactive Demo ===================== */

static void run_interactive_demo(void) {
    char ds_choice[16];
    int  n, key, start_key, end_key;
    char value[256];

    print_banner();

    /* ---- Step 1: Choose data structure ---- */
    printf("+--------------------------------------------------+\n");
    printf("|           INTERACTIVE DEMO                       |\n");
    printf("+--------------------------------------------------+\n\n");
    printf("Choose a data structure:\n");
    printf("  1. bptree   - B+ Tree\n");
    printf("  2. skiplist - Skip List\n");
    printf("  3. avl      - AVL Tree\n");
    printf("  4. bst      - Binary Search Tree\n");
    printf("\nEnter choice (bptree/skiplist/avl/bst): ");
    fflush(stdout);

    if (scanf("%15s", ds_choice) != 1) return;
    while (getchar() != '\n');   /* flush newline */

    DataStructureType type;
    if      (strcmp(ds_choice, "bptree")   == 0) type = DS_BPTREE;
    else if (strcmp(ds_choice, "skiplist") == 0) type = DS_SKIPLIST;
    else if (strcmp(ds_choice, "avl")      == 0) type = DS_AVL;
    else if (strcmp(ds_choice, "bst")      == 0) type = DS_BST;
    else {
        printf("Unknown type '%s'. Defaulting to AVL Tree.\n", ds_choice);
        type = DS_AVL;
    }

    active_type = type;
    active_ds   = adaptive_create(type);
    printf("\n>> Using: %s\n\n", ds_type_name(type));

    /* ---- Step 2: Insert ---- */
    printf("+--------------------------------------------------+\n");
    printf("|  STEP 1: INSERT                                  |\n");
    printf("+--------------------------------------------------+\n");
    printf("How many key-value pairs to insert? ");
    fflush(stdout);
    if (scanf("%d", &n) != 1 || n <= 0) { while(getchar()!='\n'); n = 0; }
    while (getchar() != '\n');

    for (int i = 0; i < n; i++) {
        printf("  Enter key (integer)  [%d/%d]: ", i + 1, n);
        fflush(stdout);
        if (scanf("%d", &key) != 1) { while(getchar()!='\n'); continue; }
        printf("  Enter value (string) [%d/%d]: ", i + 1, n);
        fflush(stdout);
        if (scanf("%255s", value) != 1) { while(getchar()!='\n'); continue; }
        while (getchar() != '\n');

        adaptive_insert(active_ds, key, value);
        printf("  --> Inserted: %d => %s\n\n", key, value);
    }

    /* ---- Show structure after inserts ---- */
    printf("\n");
    adaptive_print(active_ds);

    /* ---- Step 3: Search ---- */
    printf("+--------------------------------------------------+\n");
    printf("|  STEP 2: SEARCH                                  |\n");
    printf("+--------------------------------------------------+\n");
    printf("Enter key to search: ");
    fflush(stdout);
    if (scanf("%d", &key) == 1) {
        while (getchar() != '\n');
        char *result = adaptive_search(active_ds, key);
        if (result)
            printf("  --> Found: %d => %s\n\n", key, result);
        else
            printf("  --> Key %d NOT FOUND.\n\n", key);
    } else {
        while (getchar() != '\n');
    }

    /* ---- Step 4: Range Query ---- */
    printf("+--------------------------------------------------+\n");
    printf("|  STEP 3: RANGE QUERY                             |\n");
    printf("+--------------------------------------------------+\n");
    printf("Enter range start key: ");
    fflush(stdout);
    if (scanf("%d", &start_key) != 1) { while(getchar()!='\n'); goto skip_range; }
    printf("Enter range end   key: ");
    fflush(stdout);
    if (scanf("%d", &end_key) != 1)   { while(getchar()!='\n'); goto skip_range; }
    while (getchar() != '\n');
    do_range_query(start_key, end_key);

    skip_range:

    /* ---- Step 5: Delete ---- */
    printf("+--------------------------------------------------+\n");
    printf("|  STEP 4: DELETE                                  |\n");
    printf("+--------------------------------------------------+\n");
    printf("Enter key to delete: ");
    fflush(stdout);
    if (scanf("%d", &key) == 1) {
        while (getchar() != '\n');
        int ok = adaptive_delete(active_ds, key);
        if (ok)
            printf("  --> Deleted key %d successfully.\n\n", key);
        else
            printf("  --> Key %d NOT FOUND (nothing deleted).\n\n", key);
    } else {
        while (getchar() != '\n');
    }

    /* ---- Final structure print ---- */
    printf("+--------------------------------------------------+\n");
    printf("|  FINAL STRUCTURE STATE                           |\n");
    printf("+--------------------------------------------------+\n");
    adaptive_print(active_ds);

    adaptive_destroy(active_ds);
    active_ds = NULL;
}

/* ===================== Main ===================== */

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));

    if (argc > 1) {
        /* Command-line mode */
        if (strcmp(argv[1], "benchmark") == 0) {
            int sizes[] = {1000, 5000, 10000, 50000, 100000};
            int n = 5;
            MultiBenchmarkResult *mbr = run_multi_benchmark(sizes, n, 0);
            if (mbr) {
                print_multi_results(mbr);
                export_csv(mbr, "benchmark_random.csv");
                print_observations(mbr);
                multi_benchmark_cleanup(mbr);
            }
        } else if (strcmp(argv[1], "benchmark_sorted") == 0) {
            int sizes[] = {1000, 5000, 10000, 50000};
            int n = 4;
            MultiBenchmarkResult *mbr = run_multi_benchmark(sizes, n, 1);
            if (mbr) {
                print_multi_results(mbr);
                export_csv(mbr, "benchmark_sorted.csv");
                print_observations(mbr);
                multi_benchmark_cleanup(mbr);
            }
        } else if (strcmp(argv[1], "auto") == 0) {
            run_adaptive_demo();
        } else if (strcmp(argv[1], "theory") == 0) {
            print_theoretical_analysis();
        } else if (strcmp(argv[1], "demo") == 0) {
            run_interactive_demo();
        } else {
            printf("Usage: %s [benchmark|benchmark_sorted|auto|theory|demo]\n", argv[0]);
        }
    } else {
        /* Interactive mode */
        interactive_mode();
    }

    return EXIT_SUCCESS;
}
