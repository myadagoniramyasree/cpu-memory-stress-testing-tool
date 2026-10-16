# CPU and Memory Stress Testing Tool
### High-Performance Systems Programming Workload Generator for Linux
**OSSP Hackathon Project | Implementing Course Outcomes CO1 – CO6**

[![Language](https://img.shields.io/badge/Language-C%20(GNU11)-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform](https://img.shields.io/badge/Platform-Linux%20%2F%20POSIX-orange.svg)](https://www.kernel.org/)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](#compilation--build-instructions)
[![Test Suite](https://img.shields.io/badge/Tests-20%2F20%20Passed-success.svg)](#automated-testing)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

---

## 1. Project Overview
The **CPU and Memory Stress Testing Tool** is a robust, modular, command-line systems engineering utility written in C for Linux. It generates configurable, safe, and controlled computational and virtual-memory stress workloads, measures real-time operating system performance, monitors hardware resource utilization directly from kernel interfaces (`/proc/stat`, `/proc/meminfo`, `/proc/self/status`), and produces structured time-series CSV datasets and human-readable reports.

Developed specifically for the Operating Systems and System Programming (OSSP) hackathon, this project serves as a comprehensive, working practical implementation of all **six Course Outcomes (CO1 through CO6)**.

---

## 2. Key Features

- **Multi-Process CPU Workload Engine (CO2)**: Generates compute-intensive workloads using `fork()` with monotonic high-resolution elapsed time measurement and child process reaping via `waitpid()`.
- **External Worker Program Execution (CO1 & CO2)**: Supports both direct in-process worker execution and launching external worker programs (`stress_worker`) via `fork()` followed by `execv()`.
- **Multi-Channel Inter-Process Communication (CO3)**: Implements worker-to-controller telemetry across anonymous pipes, named pipes (FIFOs) in `/tmp`, and zero-copy POSIX shared memory in `/dev/shm`.
- **Virtual Memory Stress & Page-Fault Telemetry (CO4)**: Bounded memory allocation via `malloc()` and direct `mmap()`, with configurable sequential and random memory access patterns, read/write controls, and page-touching.
- **Copy-On-Write (COW) & Demand Paging Demo (CO4)**: An interactive verification mode demonstrating lazy physical page frame allocation and COW page duplication on `fork()`.
- **Low-Level Syscall File I/O & Structured Reports (CO5)**: Direct POSIX `open()`, `write()`, `lseek()`, and `close()` operations generating execution logs, time-series CSV datasets, text summaries, and historical ledgers.
- **POSIX Multithreading & Synchronization (CO6)**: Pthreads engine utilizing mutexes (`pthread_mutex_t`), condition variables (`pthread_cond_t`), counting semaphores (`sem_t`), and an empirical demonstration of race conditions.
- **Live Terminal Dashboard**: Clean, non-scrolling ANSI terminal status updates rendering real-time CPU %, memory used/available, resident set size (RSS), minor/major page faults, and throughput.
- **Hardware Safety Controls**: Dynamic query of `/proc/meminfo` enforcing a hard ceiling of 80% available RAM, conservative default limits, dry-run simulation mode (`--dry-run`), and clean signal traps on `SIGINT` and `SIGTERM`.

---

## 3. Course Outcomes (CO1 – CO6) Matrix

| Module / Course Outcome | Implemented Feature | Underlying OS Concepts | Primary Source Files |
| :--- | :--- | :--- | :--- |
| **CO1: OS Interface & Syscalls** | CLI parsing with `getopt_long`, dry-run mode, external worker exec | Shell command interface, user vs kernel mode (`ring 3` vs `ring 0`), `sys_execve` | `src/cli.c`, `src/config.c`, `src/worker_main.c` |
| **CO2: Process Management** | Multi-process CPU workers, process tracking, status reaping | Process creation (`fork`), scheduling, CPU affinity, zombie prevention (`waitpid`) | `src/process_manager.c`, `src/cpu_stress.c` |
| **CO3: Inter-Process Comm (IPC)** | Anonymous pipes, named pipes (FIFO), POSIX SHM, signals | Ring buffers, FIFO inodes, shared memory mapping, signal dispatch (`sigaction`, `kill`) | `src/ipc.c` |
| **CO4: Virtual Memory Management** | Bounded MB allocation, sequential vs random access, COW demo | Demand paging, page tables, TLB misses, minor/major page faults, Copy-On-Write | `src/memory_stress.c`, `src/monitor.c` |
| **CO5: File-System Operations** | Low-level file I/O, timestamped logs, CSV data, history ledger | Inodes, directory entries, file descriptor tables, VFS, `open`, `write`, `lseek` | `src/logger.c`, `src/report.c` |
| **CO6: Threads & Synchronization** | Pthreads workers, mutex locks, condition broadcast, race demo | Shared address space, TCBs, mutual exclusion, thread barriers, race hazards | `src/thread_manager.c` |

---

## 4. Repository Structure

```text
cpu-memory-stress-testing-tool/
├── README.md               # Main project documentation and user guide
├── LICENSE                 # Open-source MIT License
├── Makefile                # GNU Make build configuration
├── .gitignore              # Git ignore rules for build artifacts
├── src/                    # Implementation source files
│   ├── main.c              # Master entry point and orchestrator
│   ├── cli.c               # CLI parser, help menu, and dry-run display
│   ├── config.c            # Configuration defaults, safety limits, and validation
│   ├── cpu_stress.c        # CPU calculation kernels (Matrix, Hash, Sqrt)
│   ├── memory_stress.c     # Memory allocation, access patterns, and COW demo
│   ├── process_manager.c   # Process creation, execv, child tracking, and waitpid
│   ├── thread_manager.c    # Pthreads engine, mutex, condvar, and race demo
│   ├── ipc.c               # Anonymous pipes, FIFOs, POSIX SHM, and signals
│   ├── monitor.c           # /proc parser (/stat, /meminfo, /status) and dashboard
│   ├── logger.c            # Low-level file I/O logger (open, write, lseek, close)
│   ├── report.c            # CSV time-series, summary text, and ledger generator
│   └── worker_main.c       # Standalone external worker binary (execv target)
├── include/                # Header definitions
│   ├── cli.h
│   ├── config.h
│   ├── cpu_stress.h
│   ├── memory_stress.h
│   ├── process_manager.h
│   ├── thread_manager.h
│   ├── ipc.h
│   ├── monitor.h
│   ├── logger.h
│   └── report.h
├── tests/                  # Automated test scripts (20 test cases)
│   ├── test_config.sh      # CLI parsing and safety boundary tests
│   ├── test_cpu.sh         # Process CPU scaling and execv tests
│   ├── test_memory.sh      # Memory access and COW demo tests
│   ├── test_ipc.sh         # Pipes, FIFOs, SHM, and signal tests
│   ├── test_threads.sh     # Pthreads and race condition tests
│   └── test_reports.sh     # File system, CSV schema, and ledger tests
├── docs/                   # Detailed technical reports and evaluation guides
│   ├── project_report.md   # Full academic and engineering project report
│   ├── architecture.md     # System architecture with Mermaid diagrams
│   ├── module_mapping.md   # Six Course Outcomes detailed mapping matrix
│   ├── test_plan.md        # Comprehensive test plan and validation matrix
│   └── demo_script.md      # Step-by-step live evaluation script for evaluators
├── results/                # Output directory for logs, CSV, and summaries
└── presentation/           # PowerPoint presentation
    └── CPU_Memory_Stress_Testing_Tool.pptx
```

---

## 5. Prerequisites & Requirements

### System Requirements
- **Operating System**: Linux (Ubuntu 20.04+, Debian 11+, Fedora 36+, or WSL2 on Windows 10/11).
- **Architecture**: x86_64 or AArch64.
- **Compiler**: GCC 9.0+ or Clang 10.0+ (supporting C11/GNU11).
- **Libraries**: Standard POSIX libraries (`glibc`, `libpthread`, `librt`, `libm`).
- **Build Tools**: GNU Make, Bash.

---

## 6. Compilation & Build Instructions

Clone the repository and build using GNU Make:

```bash
# Navigate to the repository root
cd cpu-memory-stress-testing-tool

# Build both stress_tool and stress_worker binaries
make

# Display available build targets
make help

# Clean build artifacts
make clean
```

The build produces two native binaries:
1. `stress_tool`: The main controller and benchmark orchestrator.
2. `stress_worker`: The standalone external worker binary invoked via `execv()`.

---

## 7. Command-Line Reference

```text
USAGE:
  ./stress_tool [OPTIONS]

MODES (-m, --mode):
  cpu          Process-based CPU stress testing [CO2]
  memory       Virtual memory stress and page-touching [CO4]
  combined     Concurrent CPU and Memory stress testing
  threads      POSIX pthreads CPU/Memory stress testing [CO6]
  race-demo    Demonstration of Race Conditions vs Mutex locking [CO6]
  cow-demo     Demand Paging and Copy-On-Write (COW) experiment [CO4]

WORKLOAD OPTIONS:
  -w, --workers <N>      Number of worker processes or threads (default: 4, max: 64)
  -t, --time <seconds>   Duration of stress test in seconds (default: 10, max: 600)
  -b, --memory <MB>      Memory allocation limit in MB (default: 256 MB)
  -p, --pattern <pattern> Memory access pattern: sequential, random
  -o, --operation <op>   Memory operation: rw (read-write), write, read
  -A, --alloc <type>     Allocation API: malloc or mmap
  -a, --algo <algo>      CPU algorithm: matrix, hash, sqrt

SYSTEMS PROGRAMMING & OS OPTIONS:
  -e, --exec <fork|exec> Worker model: direct fork() or fork()+execv() [CO1/CO2]
  -i, --ipc <type>       IPC channel: pipe (anonymous), fifo, shm [CO3]
  -s, --sample <ms>      Monitoring sample interval in ms (default: 1000)
  --no-sync              Disable mutex in race-demo to observe data corruption [CO6]
  -d, --dry-run          Simulate configuration without stressing hardware
  -v, --verbose          Enable detailed log and runtime output
  -O, --outdir <dir>     Output directory for logs and CSV (default: 'results')
  -h, --help             Display help menu and exit
  -V, --version          Display version information
```

---

## 8. Quickstart & Execution Examples

### Example 1: Standard CPU Stress Test with Anonymous Pipes
Launch 4 process workers executing matrix multiplication for 10 seconds:
```bash
./stress_tool -m cpu -w 4 -t 10 -i pipe
```

### Example 2: External Worker Program via `fork()` + `execv()` (CO1 & CO2)
Launch external worker binary `./stress_worker` via `execv()`:
```bash
./stress_tool -m cpu -w 2 -t 5 -e exec -i pipe
```

### Example 3: Virtual Memory Stress with `mmap()` (CO4)
Allocate 512 MB virtual memory using `mmap()` and stress sequential write passes for 10 seconds:
```bash
./stress_tool -m memory -w 2 -b 512 -p sequential -A mmap -t 10
```

### Example 4: Copy-On-Write & Demand Paging Demo (CO4)
Demonstrate demand paging, page touching, and COW page frame duplication:
```bash
./stress_tool -m cow-demo -b 64
```

### Example 5: Multithreaded Workload with POSIX Threads (CO6)
Launch 8 pthreads coordinated by condition variables and semaphores:
```bash
./stress_tool -m threads -w 8 -t 10
```

### Example 6: Race Condition Bug vs. Mutex Synchronization (CO6)
Observe real data corruption caused by unsynchronized concurrent increments:
```bash
# 1. Unsynchronized (observe ~65% lost updates)
./stress_tool -m race-demo -w 4 --no-sync

# 2. Mutex Synchronized (verify 100% exact correctness)
./stress_tool -m race-demo -w 4
```

### Example 7: Dry-Run Safety Simulation
Inspect planned memory allocations and hardware topology without loading the CPU:
```bash
./stress_tool -m combined -w 8 -b 1024 --dry-run
```

---

## 9. Automated Testing Suite

The repository includes a comprehensive automated test suite consisting of 6 bash scripts exercising 20 discrete test scenarios:

```bash
make test
```

Test scripts executed:
- `tests/test_config.sh`: CLI parsing, argument validation, dry-run, and bounds checking.
- `tests/test_cpu.sh`: Single and multi-worker CPU scaling, algorithms, and `execv()` execution.
- `tests/test_memory.sh`: Sequential/random memory access, `malloc`/`mmap`, and COW demo.
- `tests/test_ipc.sh`: Anonymous pipes, named pipes (FIFOs), POSIX SHM, and `SIGINT` handling.
- `tests/test_threads.sh`: Pthreads execution, race condition detection, and mutex validation.
- `tests/test_reports.sh`: Low-level file creation, CSV time-series integrity, and history ledger.

---

## 10. Output Artifacts & Reports

Every test execution automatically generates four deliverables in the `results/` folder:

1. **Execution Log (`results/stress_<timestamp>.log`)**:
   Timestamped syscall-level log tracing process lifecycle events, IPC binding, and exit codes.
2. **Time-Series CSV Dataset (`results/stress_<timestamp>.csv`)**:
   Structured telemetry recorded at every sample interval:
   ```csv
   Timestamp_Sec,Elapsed_Sec,Mode,Workers,User_CPU_Pct,Sys_CPU_Pct,Total_CPU_Pct,Mem_Used_MB,Mem_Avail_MB,App_VmRSS_MB,App_VmSize_MB,Min_Faults,Maj_Faults,Throughput_Iter_Sec,Total_Iterations
   ```
3. **Human-Readable Summary (`results/stress_<timestamp>_summary.txt`)**:
   Executive recap detailing hardware specs, configuration, aggregate resource utilization, per-worker breakdown, and Course Outcome verification checkpoints.
4. **Historical Test Ledger (`results/test_history.csv`)**:
   Cumulative performance ledger recording each run's duration, workers, memory, average CPU %, peak RSS, iterations, and status.

---

## 11. Safety and Resource Controls

To ensure system stability during stress tests, the tool implements multiple active safeguards:
- **Available RAM Bound**: Dynamically checks `/proc/meminfo` at startup and rejects any memory request exceeding 80% of available memory, preventing the Linux OOM Killer from terminating critical services.
- **Worker Hard Bounds**: Enforces limits ($1 \le W \le 64$ workers, $1 \le T \le 600$ seconds).
- **Monotonic Clocks**: Uses `CLOCK_MONOTONIC` to ensure elapsed durations are strictly linear.
- **Graceful Signal Handling**: Traps `SIGINT` (Ctrl+C) and `SIGTERM` to immediately notify child workers, reap processes with `waitpid()`, and unlink temporary files in `/tmp` and `/dev/shm`.
- **Non-Privileged Execution**: Operates completely within standard user permissions; does not require `sudo`.

---

## 12. Team Responsibilities & Contributions

| Team Member | Academic Responsibility & Subsystem | Core Files |
| :--- | :--- | :--- |
| *[Member 1]* | Process Management & CPU Stress Engine (CO2) | `process_manager.c`, `cpu_stress.c`, `worker_main.c` |
| *[Member 2]* | Memory Stress & Virtual Memory Subsystem (CO4) | `memory_stress.c`, `monitor.c` |
| *[Member 3]* | IPC Mechanisms & Thread Synchronization (CO3, CO6) | `ipc.c`, `thread_manager.c` |
| *[Member 4]* | Resource Monitoring & Low-Level File Reports (CO5) | `logger.c`, `report.c` |
| *[Member 5]* | CLI Architecture, Integration, Docs & Presentation (CO1) | `cli.c`, `config.c`, `Makefile`, `presentation/` |

---

## 13. License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
