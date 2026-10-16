#!/bin/bash
# ==============================================================================
# test_reports.sh - Validation of Low-Level File Output, CSV Data, and Summaries
# Demonstrates CO5: File-System Operations and Performance Reports
# ==============================================================================

set -e
BIN="./stress_tool"

echo "=== [TEST 6/6] Running test_reports.sh ==="

OUTDIR="results/test_reports_out"
mkdir -p "$OUTDIR"

echo "  -> Subtest 1: Executing test run into dedicated output directory"
$BIN -m cpu -w 2 -t 2 -O "$OUTDIR" > /dev/null

echo "  -> Subtest 2: Verifying CSV dataset structure"
CSV_FILE=$(ls -1t "$OUTDIR"/stress_*.csv 2>/dev/null | head -n 1)
if [ -z "$CSV_FILE" ] || [ ! -s "$CSV_FILE" ]; then
    echo "     [FAIL] CSV dataset was not generated or is empty!"
    exit 1
fi
HEADER=$(head -n 1 "$CSV_FILE")
if echo "$HEADER" | grep -q "Timestamp_Sec,Elapsed_Sec,Mode,Workers"; then
    echo "     [PASS] CSV file valid and contains correct schema header ($CSV_FILE)"
else
    echo "     [FAIL] CSV file header is invalid: $HEADER"
    exit 1
fi

echo "  -> Subtest 3: Verifying human-readable text summary"
SUMMARY_FILE=$(ls -1t "$OUTDIR"/stress_*_summary.txt 2>/dev/null | head -n 1)
if [ -z "$SUMMARY_FILE" ] || [ ! -s "$SUMMARY_FILE" ]; then
    echo "     [FAIL] Text summary was not generated or is empty!"
    exit 1
fi
if grep -q "COURSE OUTCOME (CO1 - CO6) VERIFICATION CHECKPOINTS" "$SUMMARY_FILE"; then
    echo "     [PASS] Summary file contains Course Outcome checkpoints ($SUMMARY_FILE)"
else
    echo "     [FAIL] Summary file missing CO checkpoints!"
    exit 1
fi

echo "  -> Subtest 4: Verifying historical test ledger"
LEDGER_FILE="$OUTDIR/test_history.csv"
if [ ! -s "$LEDGER_FILE" ]; then
    echo "     [FAIL] Historical test ledger was not created!"
    exit 1
fi
echo "     [PASS] Historical ledger correctly appended ($LEDGER_FILE)"

echo "=== [PASS] test_reports.sh completed all checks successfully! ==="
