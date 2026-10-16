# Comprehensive Test Plan and Validation Strategy
## CPU and Memory Stress Testing Tool — OSSP Hackathon

---

## 1. Test Objectives & Scope
The objective of this test plan is to rigorously validate the stability, correctness, safety, and performance of the **CPU and Memory Stress Testing Tool** under diverse, controlled workloads.

The test suite exercises all six OSSP Course Outcomes:
- **CO1**: Argument parsing, command execution models, safety bounds, and dry-run simulation.
- **CO2**: Multi-process worker scalability, CPU calculation kernels, and child reaping.
- **CO3**: Anonymous pipes, FIFOs, POSIX shared memory, and asynchronous signal handling.
- **CO4**: Virtual memory allocation limits, sequential vs random access, and Copy-On-Write verification.
- **CO5**: Low-level syscall logging, CSV data integrity, and structured report generation.
- **CO6**: Multithreading with pthreads, mutex mutual exclusion, race condition detection, and condition variables.

---

## 2. Test Environment Specification
- **Hardware Architecture**: x86_64
- **Logical CPU Cores**: 8 Cores (Online)
- **Physical RAM**: 3,789 MB (Total), ~3,200 MB (Available)
- **Operating System**: Ubuntu 24.04 LTS on Linux Kernel 5.15 / 6.x (WSL2)
- **Compiler**: GCC 15.2.0 (Flags: `-Wall -Wextra -O2 -pthread -std=gnu11`)
- **Core Libraries**: `glibc 2.39`, `libpthread`, `librt`, `libm`

---

## 3. Systematic Test Execution Matrix

| Test ID | Test Category | Description & Objective | Terminal Execution Command | Expected Behavior | Actual Measured Result | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **TC-01** | CLI / CO1 | Verify version and metadata display | `./stress_tool --version` | Outputs version, edition, and returns 0 | Displays `stress_tool version 1.0.0 (OSSP Hackathon Edition)` | **PASS** |
| **TC-02** | CLI / CO1 | Verify comprehensive help message | `./stress_tool --help` | Outputs full command-line options and usage examples | Full CLI options and 8 practical examples printed | **PASS** |
| **TC-03** | Safety / CO1 | Dry-run simulation mode | `./stress_tool -m combined -w 4 -b 256 --dry-run` | Prints simulated allocation and checks safety without load | Displays system topology, planned pages (65,536), safety check PASSED | **PASS** |
| **TC-04** | Safety / CO1 | Reject invalid workload mode | `./stress_tool -m bad_mode` | Rejects argument and exits with code 1 | Displays clear error and exits with code 1 | **PASS** |
| **TC-05** | Safety / CO1 | Reject excessive workers (>64) | `./stress_tool -w 128` | Rejects worker count and exits with code 1 | Displays error `Worker count (128) must be between 1 and 64` | **PASS** |
| **TC-06** | Safety / CO1 | Reject memory request exceeding 80% RAM | `./stress_tool -m memory -b 10000` | Safety ceiling catches request; rejects execution | Error: `Requested memory exceeds safe operating ceiling` | **PASS** |
| **TC-07** | CPU / CO2 | Single process CPU workload | `./stress_tool -m cpu -w 1 -t 3 -a matrix` | Single core 100% busy; completes iterations | Completed ~75,000 iterations in 3.0s | **PASS** |
| **TC-08** | CPU / CO2 | Multi-process CPU scaling (4 workers) | `./stress_tool -m cpu -w 4 -t 3 -a matrix` | 4 cores loaded (~50% of 8 cores); clean reaping | Completed ~290,000 iterations; all children reaped | **PASS** |
| **TC-09** | Exec / CO2 | External worker binary via `fork() + execv()` | `./stress_tool -m cpu -w 2 -t 3 -e exec -i pipe` | Launches `./stress_worker` via `execv()`, reaps correctly | Both workers executed; telemetry received via pipe | **PASS** |
| **TC-10** | IPC / CO3 | Anonymous pipe data transmission | `./stress_tool -m cpu -w 2 -t 2 -i pipe` | Children transmit `worker_report_t` to parent | Parent received and logged status messages for all PIDs | **PASS** |
| **TC-11** | IPC / CO3 | Named pipe (FIFO) communication | `./stress_tool -m cpu -w 2 -t 2 -i fifo` | Creates FIFO in `/tmp`, exchanges metrics, cleans up | FIFO created, read non-blocking, unlinked on exit | **PASS** |
| **TC-12** | IPC / CO3 | POSIX Shared Memory telemetry | `./stress_tool -m cpu -w 2 -t 2 -i shm` | Real-time lockless iteration streaming via SHM | Live terminal dashboard updated iterations from SHM | **PASS** |
| **TC-13** | Signals / CO3 | Graceful termination on SIGINT | Launch background test, send `kill -INT $PID` | Parent traps signal, notifies children, reaps all | Process terminated cleanly; no orphan or zombie processes | **PASS** |
| **TC-14** | Memory / CO4 | Sequential memory write pass | `./stress_tool -m memory -w 2 -b 128 -p sequential` | Writes to 128 MB across 4KB pages; measures minor faults | RSS increased to target; ~32,768 minor page faults logged | **PASS** |
| **TC-15** | Memory / CO4 | Random memory access pass | `./stress_tool -m memory -w 2 -b 128 -p random` | Random page indexing, tests TLB and cache thrashing | Smooth execution without memory leaks or crashes | **PASS** |
| **TC-16** | COW / CO4 | Copy-On-Write & Demand Paging experiment | `./stress_tool -m cow-demo -b 64` | `mmap` zero faults -> touch 16,384 faults -> COW 8,192 faults | Step 1: 0 faults, Step 2: 16,384 faults, Step 3: 8,192 faults | **PASS** |
| **TC-17** | File IO / CO5 | Low-level file creation and CSV schema | `./stress_tool -m cpu -w 2 -t 2` | Generates `.log`, `.csv`, `_summary.txt`, `test_history.csv` | All 4 files generated with valid headers and data rows | **PASS** |
| **TC-18** | Threads / CO6 | POSIX pthreads worker engine | `./stress_tool -m threads -w 4 -t 3` | Threads coordinated by CondVar and Semaphore | 4 threads executed; 143,104 iterations completed | **PASS** |
| **TC-19** | Race / CO6 | Unsynchronized race condition bug | `./stress_tool -m race-demo -w 4 --no-sync` | Unsynchronized `counter++` loses updates | 1,391,788 lost updates (69.59% corruption observed!) | **PASS** |
| **TC-20** | Sync / CO6 | Mutex-synchronized race-free counter | `./stress_tool -m race-demo -w 4` | Mutex protects critical section; exact match | Observed: 2,000,000 / Expected: 2,000,000 (100% match) | **PASS** |

---

## 4. Edge Case and Boundary Analysis

1. **Out of Memory (OOM) Protection**:
   - The tool dynamically queries `/proc/meminfo` at startup.
   - Any allocation exceeding 80% of `MemAvailable` is immediately rejected before allocating a single byte, protecting system stability and preventing the Linux OOM Killer (`oom_reaper`) from terminating critical user processes.

2. **Zombie Process Prevention**:
   - The `process_manager_reap_children()` function loops through all child PIDs using `waitpid(pid, &status, 0)` and parses `WIFEXITED` and `WIFSIGNALED`.
   - Even when child workers finish at slightly different times, the non-blocking `waitpid(-1, &status, WNOHANG)` loop in the monitor reaps them immediately.

3. **IPC Resource Leakage Prevention**:
   - Temporary FIFOs (`/tmp/stress_tool_fifo_*`) and Shared Memory segments (`/dev/shm/stress_tool_shm_*`) are explicitly unlinked in `ipc_cleanup()` and in the signal termination handlers.

4. **Monotonic Timing Precision**:
   - Rather than relying on wall-clock time (`gettimeofday`), which can jump backward due to NTP adjustments, the tool uses `clock_gettime(CLOCK_MONOTONIC, ...)`, ensuring strictly linear elapsed time measurements.

---

## 5. Automated Regression Test Command
To run the complete automated suite of 20 test cases:
```bash
make test
```
Result: **100% Pass Rate across all 6 test suites.**
