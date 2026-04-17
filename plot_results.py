#!/usr/bin/env python3
"""
Graph Generation for Data Structure Benchmark Results

Reads CSV output from the benchmark module and generates publication-quality
graphs comparing all four data structures.

Usage:
    python plot_results.py benchmark_random.csv
    python plot_results.py benchmark_random.csv benchmark_sorted.csv

Generates:
    1. Insert Time vs Data Size
    2. Search Time vs Data Size
    3. Range Query Time vs Data Size
    4. Height vs Data Size
    5. Node Traversals vs Data Size
"""

import sys
import csv
import os
from collections import defaultdict

try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.rcParams['figure.figsize'] = (10, 6)
    matplotlib.rcParams['font.size'] = 12
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("WARNING: matplotlib not installed. Will generate text-based output only.")
    print("Install with: pip install matplotlib")


COLORS = {
    'BPlusTree': '#2196F3',
    'SkipList': '#FF9800',
    'AVLTree': '#4CAF50',
    'BST': '#F44336'
}

LABELS = {
    'BPlusTree': 'B+ Tree',
    'SkipList': 'Skip List',
    'AVLTree': 'AVL Tree',
    'BST': 'BST'
}

MARKERS = {
    'BPlusTree': 'o',
    'SkipList': 's',
    'AVLTree': '^',
    'BST': 'D'
}


def parse_csv(filename):
    """Parse benchmark CSV into structured data."""
    data = defaultdict(lambda: defaultdict(list))
    sizes = set()

    with open(filename, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            size = int(row['DataSize'])
            metric = row['Metric']
            sizes.add(size)

            for ds in ['BPlusTree', 'SkipList', 'AVLTree', 'BST']:
                val = row[ds]
                try:
                    val = float(val)
                except ValueError:
                    val = int(val)
                data[metric][ds].append((size, val))

    # Sort by size
    for metric in data:
        for ds in data[metric]:
            data[metric][ds].sort(key=lambda x: x[0])

    return data, sorted(sizes)


def plot_metric(data, metric, title, ylabel, filename, sizes):
    """Generate a single plot for one metric."""
    if not HAS_MATPLOTLIB:
        print(f"\n--- {title} ---")
        print(f"{'Size':>8}", end="")
        for ds in ['BPlusTree', 'SkipList', 'AVLTree', 'BST']:
            print(f"  {LABELS[ds]:>10}", end="")
        print()
        for i, size in enumerate(sizes):
            print(f"{size:>8}", end="")
            for ds in ['BPlusTree', 'SkipList', 'AVLTree', 'BST']:
                if i < len(data[metric][ds]):
                    print(f"  {data[metric][ds][i][1]:>10.4f}", end="")
            print()
        return

    fig, ax = plt.subplots(figsize=(10, 6))

    for ds in ['BPlusTree', 'SkipList', 'AVLTree', 'BST']:
        if ds in data[metric]:
            points = data[metric][ds]
            x = [p[0] for p in points]
            y = [p[1] for p in points]
            ax.plot(x, y, color=COLORS[ds], marker=MARKERS[ds],
                    label=LABELS[ds], linewidth=2, markersize=8)

    ax.set_xlabel('Dataset Size (N)', fontsize=13)
    ax.set_ylabel(ylabel, fontsize=13)
    ax.set_title(title, fontsize=15, fontweight='bold')
    ax.legend(fontsize=11, loc='best')
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log')

    plt.tight_layout()
    plt.savefig(filename, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"  Saved: {filename}")


def generate_all_plots(csv_file):
    """Generate all benchmark graphs from a CSV file."""
    data, sizes = parse_csv(csv_file)
    prefix = os.path.splitext(csv_file)[0]

    print(f"\nGenerating graphs from: {csv_file}")
    print(f"Dataset sizes found: {sizes}\n")

    plots = [
        ('InsertTime_ms', 'Insert Time vs Dataset Size',
         'Insert Time (ms)', f'{prefix}_insert_time.png'),
        ('SearchTime_ms', 'Search Time vs Dataset Size',
         'Search Time (ms)', f'{prefix}_search_time.png'),
        ('RangeTime_ms', 'Range Query Time vs Dataset Size',
         'Range Query Time (ms)', f'{prefix}_range_time.png'),
        ('Height', 'Structure Height vs Dataset Size',
         'Height', f'{prefix}_height.png'),
        ('Traversals', 'Node Traversals vs Dataset Size',
         'Total Node Traversals', f'{prefix}_traversals.png'),
    ]

    for metric, title, ylabel, filename in plots:
        if metric in data:
            plot_metric(data, metric, title, ylabel, filename, sizes)

    # Combined timing plot
    if HAS_MATPLOTLIB and all(m in data for m in ['InsertTime_ms', 'SearchTime_ms', 'RangeTime_ms']):
        fig, axes = plt.subplots(1, 3, figsize=(18, 5))

        for ax, (metric, title, ylabel) in zip(axes, [
            ('InsertTime_ms', 'Insert Time', 'Time (ms)'),
            ('SearchTime_ms', 'Search Time', 'Time (ms)'),
            ('RangeTime_ms', 'Range Query Time', 'Time (ms)')
        ]):
            for ds in ['BPlusTree', 'SkipList', 'AVLTree', 'BST']:
                if ds in data[metric]:
                    points = data[metric][ds]
                    x = [p[0] for p in points]
                    y = [p[1] for p in points]
                    ax.plot(x, y, color=COLORS[ds], marker=MARKERS[ds],
                            label=LABELS[ds], linewidth=2, markersize=7)
            ax.set_xlabel('Dataset Size')
            ax.set_ylabel(ylabel)
            ax.set_title(title, fontweight='bold')
            ax.legend(fontsize=9)
            ax.grid(True, alpha=0.3)
            ax.set_xscale('log')

        plt.suptitle('Performance Comparison: All Operations', fontsize=14, fontweight='bold')
        plt.tight_layout()
        combined_file = f'{prefix}_combined.png'
        plt.savefig(combined_file, dpi=150, bbox_inches='tight')
        plt.close()
        print(f"  Saved: {combined_file}")

    print("\nDone!")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python plot_results.py <csv_file> [csv_file2 ...]")
        print("Example: python plot_results.py benchmark_random.csv")
        sys.exit(1)

    for csv_file in sys.argv[1:]:
        if not os.path.exists(csv_file):
            print(f"Error: File '{csv_file}' not found.")
            print("Run the benchmark first: ./dscompare benchmark")
            continue
        generate_all_plots(csv_file)
