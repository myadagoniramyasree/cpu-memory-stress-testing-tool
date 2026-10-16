#!/bin/bash
# ==============================================================================
# test_config.sh - Validation of CLI Parsing, Boundaries, and Safety Constraints
# Demonstrates CO1: Operating System Interface and Command Execution
# ==============================================================================

set -e
BIN="./stress_tool"

echo "=== [TEST 1/6] Running test_config.sh ==="

if [ ! -x "$BIN" ]; then
    echo "Error: Binary $BIN not found or not executable. Run 'make' first."
    exit 1
fi

echo "  -> Subtest 1: Version banner"
$BIN --version > /dev/null
echo "     [PASS] --version returned 0"

echo "  -> Subtest 2: Help message"
$BIN --help > /dev/null
echo "     [PASS] --help returned 0"

echo "  -> Subtest 3: Dry-run simulation"
$BIN --dry-run > /dev/null
$BIN -m combined -w 4 -b 256 --dry-run > /dev/null
echo "     [PASS] --dry-run executed without errors"

echo "  -> Subtest 4: Reject invalid mode"
set +e
$BIN -m invalid_mode 2> /dev/null
STATUS=$?
set -e
if [ $STATUS -ne 0 ]; then
    echo "     [PASS] Rejected invalid mode correctly (Exit code: $STATUS)"
else
    echo "     [FAIL] Failed to reject invalid mode!"
    exit 1
fi

echo "  -> Subtest 5: Reject excessive worker count (>64)"
set +e
$BIN -w 999 2> /dev/null
STATUS=$?
set -e
if [ $STATUS -ne 0 ]; then
    echo "     [PASS] Rejected excessive workers correctly (Exit code: $STATUS)"
else
    echo "     [FAIL] Failed to reject excessive workers!"
    exit 1
fi

echo "  -> Subtest 6: Reject negative duration"
set +e
$BIN -t -5 2> /dev/null
STATUS=$?
set -e
if [ $STATUS -ne 0 ]; then
    echo "     [PASS] Rejected negative duration correctly (Exit code: $STATUS)"
else
    echo "     [FAIL] Failed to reject negative duration!"
    exit 1
fi

echo "  -> Subtest 7: Reject excessive memory allocation (>80% RAM)"
set +e
$BIN -m memory -b 999999 2> /dev/null
STATUS=$?
set -e
if [ $STATUS -ne 0 ]; then
    echo "     [PASS] Rejected excessive memory allocation correctly (Exit code: $STATUS)"
else
    echo "     [FAIL] Failed to reject excessive memory allocation!"
    exit 1
fi

echo "=== [PASS] test_config.sh completed all checks successfully! ==="
