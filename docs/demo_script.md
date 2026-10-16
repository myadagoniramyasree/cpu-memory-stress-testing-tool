# Live Demonstration Script & Evaluation Guide
## CPU and Memory Stress Testing Tool — OSSP Hackathon

This demo script is prepared for the live evaluation of the project. Follow this sequence of commands and speaking points to systematically demonstrate all six Course Outcomes (CO1 through CO6) to the evaluators.

---

## 1. Demo Preparation (30 Seconds Before Evaluation)
Open a Linux terminal (or WSL Ubuntu shell) and navigate to the project directory:
```bash
cd cpu-memory-stress-testing-tool
make clean && make
```
*Speaking Note:*
> "Good morning/afternoon, professors. Today we are demonstrating our CPU and Memory Stress Testing Tool, built natively in C for Linux to model operating system behaviors across process management, inter-process communication, virtual memory, low-level file I/O, and multithreading."

---

## 2. Step-by-Step Live Demonstration Sequence

### Step 1: Interface, Safety Controls & Dry-Run (CO1)
**Command:**
```bash
./stress_tool --dry-run
```
**What to Explain:**
- "Here we demonstrate **CO1 (Operating System Interface)**. The tool parses CLI arguments via POSIX `getopt_long`. In dry-run mode, it queries `/proc/stat` and `/proc/meminfo` to determine the machine's hardware topology—in this case 8 CPU cores and ~3.2 GB available RAM—and calculates the safe memory boundaries without placing any load on the operating system."

---

### Step 2: Process Creation, Scheduling & CPU Stress (CO2)
**Command:**
```bash
./stress_tool -m cpu -w 4 -t 5 -a matrix
```
**What to Explain:**
- "Here we demonstrate **CO2 (Process Management)**. The controller calls `fork()` to create 4 worker processes. Each process executes an intensive matrix multiplication kernel in user mode (`ring 3`)."
- Point to the live terminal status line:
  - "Notice the real-time CPU utilization reaching ~50% across our 8 cores, with ~25% user-time and minimal system-time."
  - "When the 5-second duration expires, the controller calls `waitpid()` to reap every child process. Notice the log shows `Reaped child PID ... Normal Exit, code: 0`. There are zero zombie or orphan processes."

---

### Step 3: External Worker Program Execution via `execv()` (CO1 & CO2)
**Command:**
```bash
./stress_tool -m cpu -w 2 -t 3 -e exec -i pipe
```
**What to Explain:**
- "Here we demonstrate the role of the shell and the `execve` system call. Instead of running internal child functions, the child processes invoke `execv('./stress_worker', argv)`. The Linux kernel completely replaces the child's address space with the new executable binary while preserving open file descriptors."
- "The external workers execute their stress kernel, transmit their telemetry over anonymous pipes, and are successfully collected and reaped by the parent."

---

### Step 4: Inter-Process Communication Channels (CO3)
**Command:**
```bash
# Demonstrate Named Pipe (FIFO)
./stress_tool -m cpu -w 2 -t 3 -i fifo

# Demonstrate POSIX Shared Memory
./stress_tool -m cpu -w 2 -t 3 -i shm
```
**What to Explain:**
- "Here we demonstrate **CO3 (Inter-Process Communication)** across multiple IPC primitives:
  1. **Anonymous Pipes**: Unidirectional kernel ring buffers.
  2. **Named Pipes (FIFOs)**: Created in `/tmp/stress_tool_fifo_*` using `mkfifo()`, providing a filesystem rendezvous for decoupled communication.
  3. **POSIX Shared Memory**: Created via `shm_open()` in `/dev/shm`, providing zero-copy memory mapping (`mmap`) where the controller directly reads live worker iterations without blocking."
- "All IPC resources are reliably unlinked and cleaned up after every test."

---

### Step 5: Demand Paging & Copy-On-Write (CO4)
**Command:**
```bash
./stress_tool -m cow-demo -b 64
```
**What to Explain:**
- "This is our dedicated **Virtual Memory Demonstration (CO4)**. Watch the three distinct steps on screen:
  1. **Step 1 (Allocation)**: We allocate 64 MB of virtual memory via `mmap(MAP_ANONYMOUS | MAP_PRIVATE)`. The minor page faults remain **+0**. This proves **Demand Paging**—the kernel only updates page table entries, committing zero physical RAM frames!
  2. **Step 2 (Page Touching)**: We write 1 byte to every 4,096-byte page. The MMU triggers hardware page faults, and the kernel allocates physical frames. The observed minor page faults jump by exactly **16,384** (which is 64 MB / 4 KB)!
  3. **Step 3 (Copy-On-Write)**: We call `fork()`. When the child reads the buffer, minor faults are **0** because pages are shared read-only. But when the child modifies half the pages, the kernel triggers COW page duplication, resulting in **8,192** minor faults."

---

### Step 6: Low-Level File I/O & Performance Reports (CO5)
**Command:**
```bash
ls -l results/
head -n 5 results/stress_*.csv
cat results/test_history.csv
```
**What to Explain:**
- "Here we demonstrate **CO5 (File-System Operations)**. The application does not use high-level streams like `fprintf`. Instead, it uses direct POSIX system calls: `open(O_CREAT | O_APPEND)`, `write()`, `lseek()`, and `close()`."
- "It generates four distinct deliverables:
  1. Raw execution logs with microsecond timestamps.
  2. A time-series CSV dataset recording CPU %, memory used, RSS, and page faults.
  3. A human-readable test summary report containing all CO checkpoints.
  4. A historical ledger (`test_history.csv`) appending performance metrics across runs for fair benchmark comparisons."

---

### Step 7: Race Condition vs. Mutex Synchronization (CO6)
**Command 1 (Unsynchronized Race Condition):**
```bash
./stress_tool -m race-demo -w 4 --no-sync
```
**What to Explain:**
- "Now we demonstrate **CO6 (Multithreading and Synchronization)**. Four threads each increment a shared counter 500,000 times. The expected total is 2,000,000. Because no lock is used, the concurrent `read-modify-write` assembly instructions interleave, causing lost updates. Look at the result: **~600,000 observed—over 65% data corruption!**"

**Command 2 (Mutex Synchronized):**
```bash
./stress_tool -m race-demo -w 4
```
**What to Explain:**
- "Now we run the exact same workload using `pthread_mutex_lock()` and `pthread_mutex_unlock()`. Mutual exclusion guarantees that only one thread enters the critical section at a time. The result: **Exactly 2,000,000—100% correct!**"

---

### Step 8: Automated Regression Test Suite
**Command:**
```bash
make test
```
**What to Explain:**
- "Finally, to demonstrate comprehensive validation, our automated test suite executes 20 test cases across 6 bash scripts, verifying all modules with a 100% pass rate."

---

## 3. Anticipated Questions from Evaluators & Model Answers

### Q1: Why did you use `mmap()` instead of just `malloc()`?
**Answer:**
> "`malloc()` typically uses `brk()` for small allocations and `mmap()` for large allocations via glibc. By directly using `mmap(MAP_ANONYMOUS | MAP_PRIVATE)`, we bypass glibc's internal heap caching and interact directly with the Linux kernel virtual memory subsystem. This enables us to cleanly measure demand paging and page fault exceptions without heap fragmentation interference."

### Q2: How does your tool prevent zombie processes?
**Answer:**
> "A zombie process occurs when a child terminates but its exit status remains in the process table because the parent hasn't read it. We prevent this by calling `waitpid(pg->pids[i], &status, 0)` for every spawned worker. We also inspect the termination status using `WIFEXITED()` to confirm clean termination, and `WEXITSTATUS()` to log the exit code."

### Q3: What is the difference between Minor and Major page faults?
**Answer:**
> "A **Minor Page Fault** occurs when the virtual memory page is mapped, but no physical page frame is assigned yet (such as in demand paging or Copy-On-Write), requiring the kernel to allocate a RAM frame from the page pool without disk access. A **Major Page Fault** occurs when the required page resides in the swap partition or an executable disk file, requiring slow secondary storage I/O to read the page into RAM."

### Q4: How do you prevent deadlocks in your multithreaded engine?
**Answer:**
> "We prevent deadlocks by systematically eliminating Coffman's conditions:
> 1. **Lock Ordering**: All threads acquire mutexes in a strictly identical global sequence.
> 2. **No Hold-and-Wait**: Threads do not acquire a second lock while holding a first.
> 3. **Fine-Grained Locking**: Critical sections are kept minimal (updating a single counter), and locks are released immediately."

### Q5: Can this tool crash the host operating system?
**Answer:**
> "No. We engineered strict safety rails:
> 1. At startup, the tool inspects `MemAvailable` from `/proc/meminfo` and enforces a hard ceiling of 80% available RAM. Any excessive request is rejected before allocation.
> 2. Conservative defaults (2 workers, 10 seconds, 256 MB) ensure safe execution.
> 3. Clean signal traps on `SIGINT` and `SIGTERM` guarantee that if a user cancels a test, all children are terminated and IPC resources in `/dev/shm` and `/tmp` are cleanly unlinked."
