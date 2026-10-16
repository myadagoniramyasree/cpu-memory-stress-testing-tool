#!/bin/bash
# ==============================================================================
# test_memory.sh - Validation of Memory Stress, Allocation APIs, and COW Demo
# Demonstrates CO4: Memory Stress and Virtual Memory
# ==============================================================================

set -e
BIN="./stress_tool"

echo "=== [TEST 3/6] Running test_memory.sh ==="

echo "  -> Subtest 1: Sequential memory access (64 MB, malloc, 2s)"
$BIN -m memory -w 2 -b 64 -p sequential -A malloc -t 2 > /dev/null
echo "     [PASS] Sequential malloc test completed"

echo "  -> Subtest 2: Random memory access (64 MB, mmap, 2s)"
$BIN -m memory -w 2 -b 64 -p random -A mmap -t 2 > /dev/null
echo "     [PASS] Random mmap test completed"

echo "  -> Subtest 3: Memory read-only operation"
$BIN -m memory -w 1 -b 32 -o read -t 2 > /dev/null
echo "     [PASS] Memory read-only test completed"

echo "  -> Subtest 4: Educational Demand Paging & Copy-On-Write experiment"
$BIN -m cow-demo -b 32 > /dev/null
echo "     [PASS] Copy-On-Write demo completed"

echo "=== [PASS] test_memory.sh completed all checks successfully! ==="
