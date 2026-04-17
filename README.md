# Comparative Analysis and Adaptive Selection of Data Structures for Efficient Range Queries

A comprehensive C project implementing and benchmarking four ordered data structures, with an intelligent adaptive selection system that automatically chooses the best structure for a given workload.

---

## 📋 Table of Contents
1. [Project Overview](#project-overview)
2. [Data Structures Implemented](#data-structures-implemented)
3. [Project Structure](#project-structure)
4. [Building & Running](#building--running)
5. [Command Reference](#command-reference)
6. [Benchmark Results](#benchmark-results)
7. [Graph Analysis](#graph-analysis)
8. [Theoretical Analysis](#theoretical-analysis)
9. [Adaptive Selection System](#adaptive-selection-system)
10. [Observations & Conclusions](#observations--conclusions)

---

## Project Overview

This project fulfills the following goals:
- **Phase 1**: Implement all four data structures with insert, search, delete, and range query operations
- **Benchmarking**: Measure performance across dataset sizes 1K–100K in both random and sorted input modes
- **Graph Generation**: Export CSV data and generate publication-quality graphs via Python/matplotlib
- **Observation Engine**: Automatically derive insights from benchmark results
- **Phase 2**: Adaptive workload-aware selection system using weighted scoring

---

## Data Structures Implemented

### 1. B+ Tree (`src/bptree.c`)
- **Design**: Disk-style B+ Tree with configurable order (fan-out). All data stored in leaf nodes; internal nodes contain routing keys only
- **Key Feature**: Leaf nodes are **doubly-linked** for O(k) range traversal after an O(log n) descent
- **Implementation Notes**:
  - Parent-path stack tracked during descent (eliminates fragile pointer offset arithmetic)
  - Correct merge-then-split approach for both leaf and internal node splits
  - Dynamic BFS queue for safe visualization of very large trees
- **Operations**: Insert O(log n), Search O(log n), Delete O(log n), Range Query O(log n + k)

### 2. Skip List (`src/skiplist.c`)
- **Design**: Probabilistic multi-level linked list (P=0.5, max 32 levels)
- **Key Feature**: No rotations needed; probabilistic balancing through random level assignment
- **Operations**: Insert O(log n) avg, Search O(log n) avg, Delete O(log n) avg, Range Query O(log n + k) avg

### 3. AVL Tree (`src/avl.c`)
- **Design**: Self-balancing BST; height difference between left/right subtrees ≤ 1
- **Key Feature**: Strict balance guarantee via four rotation types (LL, RR, LR, RL)
- **Operations**: Insert O(log n), Search O(log n), Delete O(log n), Range Query O(log n + k)

### 4. Binary Search Tree (`src/bst.c`)
- **Design**: Basic unbalanced BST, intentionally kept simple as a comparison baseline
- **Key Feature**: Demonstrates worst-case degradation on sorted input (becomes a linked list)
- **Implementation Notes**: Fully iterative insert, search, delete, height, range query, and destroy to prevent stack overflow on degenerate (height=N) trees
- **Operations**: Insert O(log n) avg / O(n) worst, Search O(log n) avg / O(n) worst

---

## Project Structure

```
ADS_CP/
├── main.c                   # CLI entry point & interactive shell
├── Makefile                 # Build system
├── plot_results.py          # Graph generation (Python/matplotlib)
├── README.md                # This file
│
├── src/
│   ├── bptree.c             # B+ Tree implementation
│   ├── skiplist.c           # Skip List implementation
│   ├── avl.c                # AVL Tree implementation
│   ├── bst.c                # BST implementation (fully iterative)
│   ├── benchmark.c          # Benchmarking module & observation engine
│   └── adaptive.c           # Adaptive selection system
│
└── include/
    ├── bptree.h
    ├── skiplist.h
    ├── avl.h
    ├── bst.h
    ├── benchmark.h
    └── adaptive.h
```

---

## Building & Running

### Prerequisites
- GCC (C99 or later)
- Python 3 + matplotlib (optional, for graphs)

### Build
```bash
# Using make
make

# Or directly with gcc
gcc -Wall -Wextra -O2 -std=c99 -Iinclude -o dscompare \
    main.c src/bptree.c src/skiplist.c src/avl.c src/bst.c \
    src/benchmark.c src/adaptive.c -lm
```

### Quick Run Options
```bash
./dscompare                  # Interactive CLI
./dscompare demo             # Demo with 20 sample items
./dscompare benchmark        # Full random benchmark (1K–100K)
./dscompare benchmark_sorted # Sorted input benchmark (1K–50K)
./dscompare auto             # Adaptive selection demo
./dscompare theory           # Theoretical complexity table

# Generate graphs from CSV
python plot_results.py benchmark_random.csv
python plot_results.py benchmark_random.csv benchmark_sorted.csv
```

---

## Command Reference

| Command | Description |
|---------|-------------|
| `insert <key> <value>` | Insert key-value pair into active structure |
| `search <key>` | Point lookup |
| `delete <key>` | Remove a key |
| `range <start> <end>` | Range query (inclusive) |
| `benchmark` | Full random-input benchmark across 5 sizes |
| `benchmark_sorted` | Sorted-input benchmark across 4 sizes |
| `benchmark_quick` | Quick benchmark (1K, 5K, 10K only) |
| `auto` | Run adaptive selection demo across 6 workload profiles |
| `auto_custom` | Enter custom workload parameters |
| `theory` | Print complexity table |
| `switch <bptree\|skiplist\|avl\|bst>` | Change active data structure |
| `print_structure` | Print current DS contents |
| `load_sample` | Load 20 fruit-named sample items |
| `help` | Show all commands |
| `exit` / `quit` | Exit |

---

## Benchmark Results

### Random Input (1K – 100K elements)

#### Insert Time (ms)

| N | B+ Tree | Skip List | AVL Tree | BST |
|---|---------|-----------|----------|-----|
| 1,000 | 2.0 | 2.0 | 0.0 | 2.0 |
| 5,000 | 8.0 | 8.0 | 6.0 | 10.0 |
| 10,000 | 18.0 | 14.0 | 16.0 | 17.0 |
| 50,000 | 90.0 | 96.0 | 89.0 | 90.0 |
| 100,000 | 207.0 | 200.0 | 181.0 | 166.0 |

#### Search Time (ms)

| N | B+ Tree | Skip List | AVL Tree | BST |
|---|---------|-----------|----------|-----|
| 50,000 | 11.0 | 10.0 | **8.0** | 5.0 |
| 100,000 | 24.0 | 28.0 | **9.0** | 13.0 |

#### Height Comparison

| N | B+ Tree | Skip List | AVL Tree | BST |
|---|---------|-----------|----------|-----|
| 1,000 | 7 | 8 | 12 | 21 |
| 10,000 | 9 | 13 | 16 | 30 |
| 100,000 | **11** | 16 | 20 | 41 |

### Sorted Input (demonstrates BST degradation)

| N | B+ Tree Height | AVL Height | BST Height |
|---|----------------|------------|------------|
| 1,000 | 7 | 10 | **1,000** |
| 5,000 | 8 | 13 | **5,000** |
| 10,000 | 9 | 14 | **10,000** |
| 50,000 | 10 | 16 | **50,000** |

BST insert time (sorted, N=50,000): **3,661 ms** vs AVL: **97 ms** — a **37.7× slowdown**.

---

## Graph Analysis

The following PNG graphs are generated by `plot_results.py`:

| File | Description |
|------|-------------|
| `benchmark_random_insert_time.png` | Insert time vs dataset size (random) |
| `benchmark_random_search_time.png` | Search time vs dataset size (random) |
| `benchmark_random_range_time.png` | Range query time vs dataset size (random) |
| `benchmark_random_height.png` | Tree height vs dataset size (random) |
| `benchmark_random_traversals.png` | Node traversals vs dataset size (random) |
| `benchmark_random_combined.png` | All three operation times side-by-side |
| `benchmark_sorted_*.png` | Same for sorted input |

**Key graph observations**:
- **Height graph**: B+ Tree grows logarithmically with the highest fan-out (order=4), giving the lowest height. BST spikes to N for sorted input.
- **Insert time (sorted)**: BST curves shoot up quadratically (O(n) per insert × N inserts = O(n²)) while all others stay near-linear.
- **Range query**: B+ Tree and Skip List achieve sub-millisecond range queries due to linked-list traversal after initial descent.

---

## Theoretical Analysis

| Operation | B+ Tree | Skip List | AVL Tree | BST |
|-----------|---------|-----------|----------|-----|
| Insert Best | O(log n) | O(log n) | O(log n) | O(log n) |
| Insert Avg | O(log n) | O(log n) | O(log n) | O(log n) |
| Insert Worst | O(log n) | O(n)* | O(log n) | **O(n)** |
| Search Best | O(log n) | O(log n) | O(log n) | O(1) |
| Search Avg | O(log n) | O(log n) | O(log n) | O(log n) |
| Search Worst | O(log n) | O(n)* | O(log n) | **O(n)** |
| Range Query | O(log n + k) | O(log n + k) | O(log n + k) | O(n + k) |
| Space | O(n) | O(n log n) | O(n) | O(n) |
| Height | O(log_m n) | O(log n) exp | O(log n) | O(n) worst |

*Skip List O(n) worst case occurs with probability ~(1/2)^n

### Behavior Under Sorted Input
- **B+ Tree**: Unaffected. Splits distribute keys evenly. Height = O(log_m n).
- **Skip List**: Input order irrelevant. Probabilistic levels maintain expected O(log n).
- **AVL Tree**: Handles sorted input with up to O(log n) rotations per insert. Height kept ≤ 1.44 log₂(n).
- **BST**: **Degrades to O(n) per operation**. Sorted inserts create a linear chain. Height = N.

---

## Adaptive Selection System

The `select_data_structure(WorkloadProfile)` function uses a weighted scoring model:

```c
typedef struct {
    int dataset_size;
    double insert_ratio;    // fraction of ops that are inserts
    double search_ratio;    // fraction of ops that are searches
    double range_ratio;     // fraction of ops that are range queries
    double delete_ratio;    // fraction that are deletes
    double data_randomness; // 0.0 = sorted, 1.0 = random
    int range_query_size;   // avg span of range queries
} WorkloadProfile;
```

**Scoring Dimensions** (each 0–10 per structure):
1. **Range query affinity** (3× weight): B+ Tree >> Skip List > AVL > BST
2. **Insert affinity** (2× weight): Skip List > BST (random) > B+ Tree ≈ AVL; BST heavily penalized on sorted data
3. **Search affinity** (2× weight): AVL > B+ Tree > Skip List ≈ BST
4. **Dataset size bonus**: BST for <1K; AVL/Skip for 1K–10K; B+ Tree for >50K
5. **Sorted data penalty**: BST receives -5 score when randomness < 30%

**Example selections**:

| Workload | Selected | Confidence |
|----------|----------|------------|
| 60% range queries, N=10K | **B+ Tree** | 34% |
| 60% random inserts, N=10K | **Skip List** | 36% |
| 70% searches, N=10K | **B+ Tree** | 31% |
| N=500, mixed | **B+ Tree** | 27% |
| 60% range queries, N=100K | **B+ Tree** | 40% |

---

## Observations & Conclusions

### Automatically Generated Observations (Random Input, N=100K)
1. **B+ Tree** performs best for range queries (linked leaf chain eliminates tree traversal)
2. **BST** has fastest raw insert for random data (no rebalancing overhead) but doesn't scale
3. **BST height (41)** is 2× AVL height (20) even with random input — no balance guarantees
4. **AVL Tree** achieves fastest search (9ms for 100K lookups) with strict O(log n) guarantee
5. **B+ Tree** achieves minimum height (11) at N=100K: lowest traversal depth per search
6. **Scaling** (insert, N=1K → N=100K): B+ Tree 103×, BST 83× — comparable with random input

### Conclusions
| Recommendation | When to Use |
|---------------|-------------|
| **B+ Tree** | Frequent range queries; large datasets (>50K); database-style access patterns |
| **Skip List** | High-throughput random inserts; concurrent workloads; simplicity desired |
| **AVL Tree** | Search-heavy workloads requiring strict O(log n) guarantees; balanced operations |
| **BST** | Very small datasets (<1K); educational purposes; guaranteed random input only |

> **Never use BST for sorted or nearly-sorted input** — the implementation becomes a degenerate O(n) structure.

---

*Built with C99 | Benchmarked on Windows | Graphs generated with Python 3 + matplotlib*
