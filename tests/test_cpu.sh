#!/bin/bash
# ==============================================================================
# test_cpu.sh - Validation of CPU Stress Workload and Worker Scaling
# Demonstrates CO2: Process Management and CPU Stress
# ==============================================================================

set -e
BIN="./stress_tool"

echo "=== [TEST 2/6] Running test_cpu.sh ==="

echo "  -> Subtest 1: Single process worker (Duration: 2s)"
$BIN -m cpu -w 1 -t 2 -a matrix -i pipe > /dev/null
echo "     [PASS] Single worker test completed"

echo "  -> Subtest 2: Multi-process workers scaling (4 workers, Duration: 2s)"
$BIN -m cpu -w 4 -t 2 -a hash -i pipe > /dev/null
echo "     [PASS] 4-worker CPU test completed"

echo "  -> Subtest 3: Alternative CPU algorithm (Square Root & Transcendentals)"
$BIN -m cpu -w 2 -t 2 -a sqrt -i pipe > /dev/null
echo "     [PASS] Sqrt/Trans CPU algorithm test completed"

echo "  -> Subtest 4: External worker binary execution via fork() + execv()"
$BIN -m cpu -w 2 -t 2 -e exec -i pipe > /dev/null
echo "     [PASS] fork() + execv() external worker execution completed"

echo "=== [PASS] test_cpu.sh completed all checks successfully! ==="
