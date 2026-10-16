#!/bin/bash
# ==============================================================================
# test_threads.sh - Validation of Multithreading, Synchronization, and Race Demos
# Demonstrates CO6: Multithreading and Synchronization
# ==============================================================================

set -e
BIN="./stress_tool"

echo "=== [TEST 5/6] Running test_threads.sh ==="

echo "  -> Subtest 1: POSIX pthreads CPU workload (4 threads, Duration: 2s)"
$BIN -m threads -w 4 -t 2 > /dev/null
echo "     [PASS] Pthreads 4-worker workload completed"

echo "  -> Subtest 2: Thread Race Condition Demo without Mutex (Expect lost updates)"
OUTPUT_UNSYNC=$($BIN -m race-demo -w 4 --no-sync)
if echo "$OUTPUT_UNSYNC" | grep -q "RACE CONDITION CONFIRMED"; then
    echo "     [PASS] Unsynchronized execution correctly detected race condition"
else
    echo "     [FAIL] Failed to observe race condition!"
    exit 1
fi

echo "  -> Subtest 3: Thread Race Condition Demo with Mutex (Expect 100% match)"
OUTPUT_SYNC=$($BIN -m race-demo -w 4)
if echo "$OUTPUT_SYNC" | grep -q "Counter matches expected total exactly"; then
    echo "     [PASS] Mutex synchronization verified 100% correct"
else
    echo "     [FAIL] Mutex synchronized execution did not match expected total!"
    exit 1
fi

echo "=== [PASS] test_threads.sh completed all checks successfully! ==="
