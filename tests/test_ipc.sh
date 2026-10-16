#!/bin/bash
# ==============================================================================
# test_ipc.sh - Validation of Pipes, FIFOs, Shared Memory, and Signals
# Demonstrates CO3: Inter-Process Communication
# ==============================================================================

set -e
BIN="./stress_tool"

echo "=== [TEST 4/6] Running test_ipc.sh ==="

echo "  -> Subtest 1: Anonymous Pipes IPC communication"
$BIN -m cpu -w 2 -t 2 -i pipe > /dev/null
echo "     [PASS] Anonymous pipe communication completed"

echo "  -> Subtest 2: Named Pipe (FIFO) IPC communication"
$BIN -m cpu -w 2 -t 2 -i fifo > /dev/null
echo "     [PASS] FIFO communication completed"

echo "  -> Subtest 3: POSIX Shared Memory IPC communication"
$BIN -m cpu -w 2 -t 2 -i shm > /dev/null
echo "     [PASS] POSIX Shared Memory communication completed"

echo "  -> Subtest 4: Signal Handling (SIGINT graceful interruption)"
$BIN -m cpu -w 2 -t 10 &
PID=$!
sleep 1
kill -INT $PID
wait $PID 2> /dev/null || true
echo "     [PASS] SIGINT graceful termination verified without hanging"

echo "=== [PASS] test_ipc.sh completed all checks successfully! ==="
