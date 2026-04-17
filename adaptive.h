#ifndef ADAPTIVE_H
#define ADAPTIVE_H

/*
 * Adaptive Data Structure Selection System
 *
 * This module implements a rule-based system that automatically selects
 * the best data structure based on workload characteristics.
 *
 * Inputs considered:
 * - Dataset size
 * - Ratio of operations (insert/search/range)
 * - Frequency of range queries
 * - Data randomness (0.0 = fully sorted, 1.0 = fully random)
 *
 * Selection Rules:
 *   IF range_queries are high          → B+ Tree
 *   IF inserts are highly random       → Skip List
 *   IF dataset is small (<1000)        → BST
 *   IF balanced performance needed     → AVL Tree
 *   ELSE                               → AVL Tree (safe default)
 */

/* Data structure type enum */
typedef enum {
    DS_BPTREE = 0,
    DS_SKIPLIST,
    DS_AVL,
    DS_BST,
    DS_COUNT  /* Number of data structure types */
} DataStructureType;

/* Workload profile describing the expected usage pattern */
typedef struct {
    int dataset_size;           /* Expected number of elements */
    double insert_ratio;        /* Fraction of operations that are inserts (0.0 - 1.0) */
    double search_ratio;        /* Fraction of operations that are searches */
    double range_ratio;         /* Fraction of operations that are range queries */
    double delete_ratio;        /* Fraction of operations that are deletes */
    double data_randomness;     /* 0.0 = fully sorted, 1.0 = fully random */
    int range_query_size;       /* Average range query span */
} WorkloadProfile;

/* Selection result with explanation */
typedef struct {
    DataStructureType selected;
    const char *reason;
    double confidence;          /* 0.0 - 1.0 confidence in selection */
    double scores[DS_COUNT];    /* Score for each data structure */
} SelectionResult;

/* Unified interface for any data structure */
typedef struct {
    DataStructureType type;
    void *ds;                   /* Pointer to actual data structure */
    const char *name;
} AdaptiveDS;

/* Core Functions */
SelectionResult select_data_structure(WorkloadProfile profile);
const char* ds_type_name(DataStructureType type);
void print_workload_profile(WorkloadProfile *profile);
void print_selection_result(SelectionResult *result);

/* Adaptive DS wrapper operations */
AdaptiveDS* adaptive_create(DataStructureType type);
void adaptive_insert(AdaptiveDS *ads, int key, const char *value);
char* adaptive_search(AdaptiveDS *ads, int key);
int adaptive_delete(AdaptiveDS *ads, int key);
void adaptive_print(AdaptiveDS *ads);
void adaptive_destroy(AdaptiveDS *ads);

/* Predefined workload profiles for testing */
WorkloadProfile workload_heavy_range(int size);
WorkloadProfile workload_heavy_insert(int size);
WorkloadProfile workload_heavy_search(int size);
WorkloadProfile workload_mixed(int size);
WorkloadProfile workload_small_dataset(void);

#endif /* ADAPTIVE_H */
