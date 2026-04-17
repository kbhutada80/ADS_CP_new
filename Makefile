# Makefile for Comparative Analysis of Data Structures
#
# Targets:
#   make          - Build the executable
#   make clean    - Remove compiled files
#   make run      - Build and run interactive mode
#   make demo     - Run demo mode
#   make benchmark - Run full benchmark
#   make benchmark_sorted - Run sorted input benchmark
#   make auto     - Run adaptive selection demo
#   make theory   - Print theoretical analysis
#   make plots    - Generate graphs from CSV (requires Python)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

# Directories
SRC_DIR = src
INC_DIR = include

# Output binary
TARGET = dscompare

# Source files
SOURCES = main.c \
          $(SRC_DIR)/bptree.c \
          $(SRC_DIR)/skiplist.c \
          $(SRC_DIR)/avl.c \
          $(SRC_DIR)/bst.c \
          $(SRC_DIR)/benchmark.c \
          $(SRC_DIR)/adaptive.c

HEADERS = $(INC_DIR)/bptree.h \
          $(INC_DIR)/skiplist.h \
          $(INC_DIR)/avl.h \
          $(INC_DIR)/bst.h \
          $(INC_DIR)/benchmark.h \
          $(INC_DIR)/adaptive.h

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
	@echo "Build successful: $(TARGET)"

# Compile source files
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) -c $< -o $@ $(CFLAGS) -I$(INC_DIR)

# Run interactive mode
run: $(TARGET)
	@./$(TARGET)

# Run demo mode
demo: $(TARGET)
	@./$(TARGET) demo

# Run full benchmark (random input)
benchmark: $(TARGET)
	@./$(TARGET) benchmark

# Run sorted input benchmark
benchmark_sorted: $(TARGET)
	@./$(TARGET) benchmark_sorted

# Run adaptive selection demo
auto: $(TARGET)
	@./$(TARGET) auto

# Print theoretical analysis
theory: $(TARGET)
	@./$(TARGET) theory

# Generate plots from CSV (requires Python 3 + matplotlib)
plots: benchmark
	@echo "Generating plots..."
	python plot_results.py benchmark_random.csv
	@echo "Plots saved."

# Clean build artifacts
clean:
	@echo "Cleaning..."
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe *.csv
	@echo "Clean complete."

# Help target
help:
	@echo ""
	@echo "Comparative Analysis of Data Structures - Build Targets"
	@echo "======================================================="
	@echo ""
	@echo "  make              Build the executable"
	@echo "  make run          Build and run interactive CLI"
	@echo "  make demo         Run demonstration"
	@echo "  make benchmark    Run full benchmark (random input)"
	@echo "  make benchmark_sorted  Run benchmark (sorted input)"
	@echo "  make auto         Run adaptive selection demo"
	@echo "  make theory       Print theoretical analysis"
	@echo "  make plots        Generate graphs (needs Python)"
	@echo "  make clean        Remove build artifacts"
	@echo ""

.PHONY: all run demo benchmark benchmark_sorted auto theory plots clean help
