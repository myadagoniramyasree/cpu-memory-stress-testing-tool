# Course Outcomes (CO1 – CO6) Mapping Matrix
## CPU and Memory Stress Testing Tool — OSSP Hackathon

This document provides a comprehensive mapping between the six academic Course Outcomes (CO1 through CO6) of the Operating Systems & System Programming (OSSP) curriculum and the practical engineering implementation inside the `cpu-memory-stress-testing-tool`.

---

## Comprehensive CO Mapping Table

| Course Outcome | Academic Definition | Implemented Features & Subsystems | Relevant OS Concepts | Source Files & Key Functions | Verification & Evidence |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CO1** | **Operating System Interface and Command Execution** | - Terminal CLI with POSIX `getopt_long`<br>- Dry-run safety simulator<br>- Invocation of external worker binary (`stress_worker`)<br>- Syscall tracing with `strace`<br>- Error handling & argument validation | - Shell command line interface<br>- User mode vs Kernel mode (`ring 3` vs `ring 0`)<br>- System call execution traps (`sys_execve`)<br>- System-wide error numbers (`errno`, `strerror`) | `src/cli.c`<br>`src/config.c`<br>`src/worker_main.c`<br>`cli_parse()`<br>`cli_print_dry_run()` | `./stress_tool --help`<br>`./stress_tool --dry-run`<br>`./stress_tool -e exec`<br>`bash tests/test_config.sh` |
| **CO2** | **Process Management and CPU Stress** | - Configurable CPU worker generation<br>- Child process tracking (PIDs)<br>- Process reaping and exit code parsing<br>- Monotonic elapsed time calculation<br>- Math, Hash, & Sqrt computation kernels | - Process creation (`sys_clone` / `fork`)<br>- Child execution (`execv`)<br>- Process termination (`exit`, `_exit`)<br>- Zombie process prevention (`waitpid`)<br>- Exit status macros (`WIFEXITED`, `WEXITSTATUS`, `WTERMSIG`)<br>- CPU scheduling & time slicing | `src/process_manager.c`<br>`src/cpu_stress.c`<br>`process_manager_run()`<br>`process_manager_reap_children()`<br>`cpu_kernel_matrix_mult()` | `./stress_tool -m cpu -w 4 -t 5`<br>`bash tests/test_cpu.sh`<br>Exit code logging in summary report |
| **CO3** | **Inter-Process Communication (IPC)** | - Anonymous unidirectional pipes<br>- Named Pipes (FIFOs) rendezvous<br>- POSIX Shared Memory segments<br>- Asynchronous signal trapping (`SIGINT`, `SIGTERM`, `SIGALRM`)<br>- Coordinated resource cleanup | - IPC channels and data buffering<br>- Kernel pipe ring buffer (`PIPE_BUF`)<br>- Named pipe inode representation<br>- Shared address space mapping (`shm_open`, `mmap`)<br>- Software interrupt signals (`sigaction`)<br>- Process groups and signal dispatch (`kill`) | `src/ipc.c`<br>`ipc_init()`<br>`ipc_pipe_send_report()`<br>`ipc_fifo_send_report()`<br>`ipc_shm_update_worker()`<br>`ipc_setup_signal_handlers()` | `./stress_tool -i pipe`<br>`./stress_tool -i fifo`<br>`./stress_tool -i shm`<br>`bash tests/test_ipc.sh`<br>Ctrl+C clean termination |
| **CO4** | **Memory Stress and Virtual Memory** | - Bounded physical & virtual memory allocation<br>- Sequential & random access patterns<br>- Controlled Read/Write operations<br>- Real-time page fault monitoring<br>- Interactive Copy-On-Write (COW) experiment | - Virtual vs Physical address space<br>- Paging & Page Tables (`sysconf(_SC_PAGESIZE)`)<br>- Translation Lookaside Buffer (TLB) misses<br>- Demand Paging & Lazy Allocation<br>- Page Fault Exceptions (Minor vs Major)<br>- Copy-On-Write (COW) page frame sharing | `src/memory_stress.c`<br>`src/monitor.c`<br>`memory_stress_worker_loop()`<br>`memory_run_cow_experiment()`<br>`memory_access_sequential()`<br>`memory_access_random()` | `./stress_tool -m cow-demo -b 64`<br>`./stress_tool -m memory -b 256 -p seq`<br>`./stress_tool -m memory -b 256 -p rand`<br>`bash tests/test_memory.sh` |
| **CO5** | **File-System Operations and Performance Reports** | - Low-level syscall file I/O (`open`, `write`, `lseek`, `close`)<br>- Automatic directory creation (`mkdir`)<br>- Time-series CSV dataset output<br>- Human-readable summary file<br>- Historical test ledger (`test_history.csv`) | - Inodes and Directory Entries (dentries)<br>- Process File Descriptor Table (FDT)<br>- System Open File Description Table<br>- Virtual File System (VFS) abstraction<br>- File offset pointer manipulation (`lseek`)<br>- POSIX file creation modes (`0644`, `0755`) | `src/logger.c`<br>`src/report.c`<br>`logger_init()`<br>`log_message()`<br>`report_csv_append()`<br>`report_generate_summary()`<br>`report_update_history_ledger()` | `cat results/stress_*_summary.txt`<br>`cat results/stress_*.csv`<br>`cat results/test_history.csv`<br>`bash tests/test_reports.sh` |
| **CO6** | **Multithreading and Synchronization** | - POSIX pthreads worker engine (`pthread_create`, `pthread_join`)<br>- Mutex lock protection (`pthread_mutex_t`)<br>- Condition variable barrier start (`pthread_cond_t`)<br>- Counting Semaphore slot throttling (`sem_t`)<br>- Race Condition demonstration mode | - Kernel threads vs User threads<br>- Thread Control Blocks (TCB) & shared heap<br>- Critical section mutual exclusion<br>- Thread rendezvous & broadcast barriers<br>- Bounded buffer counting semaphores<br>- Data race corruption vs Atomic correctness<br>- Deadlock prevention (Coffman conditions) | `src/thread_manager.c`<br>`thread_manager_run()`<br>`thread_manager_run_race_demo()`<br>`thread_worker_routine()` | `./stress_tool -m race-demo -w 4 --no-sync`<br>`./stress_tool -m race-demo -w 4`<br>`./stress_tool -m threads -w 4 -t 5`<br>`bash tests/test_threads.sh` |

---

## In-Depth Analysis of Operating System Concepts by Module

### Module 1: OS Interface & Syscalls (CO1)
- **User Mode vs. Kernel Mode Transition**:
  When a user executes `./stress_tool`, execution begins in user mode (CPU Privilege Ring 3). System calls like `open()`, `write()`, and `fork()` invoke assembly `syscall` instructions that trigger a hardware trap into Kernel Mode (Privilege Ring 0).
- **Execution of External Worker Programs (`execv`)**:
  When invoked with `-e exec`, the child process replaces its entire address space, code segment, and heap with the binary image of `./stress_worker` via the `execve` system call, demonstrating the fundamental two-step process creation architecture of UNIX.

### Module 2: Process Management (CO2)
- **Process Creation and Isolation**:
  `fork()` creates an exact clone of the caller's address space. The operating system kernel allocates a new Process Control Block (PCB), assigns a unique Process ID (PID), and schedules the child independently.
- **Process Reaping & Zombie Prevention**:
  When a child exits, it transitions into a `zombie` state until its parent reads its termination status via `waitpid()`. `stress_tool` guarantees 100% reaping of all child processes, logging their termination reasons via `WIFEXITED(status)` and `WEXITSTATUS(status)`.

### Module 3: Inter-Process Communication (CO3)
- **Anonymous Pipes**:
  Created via `pipe(pipefd)`. Backed by an in-memory kernel ring buffer (typically 64 KB). The child writes a binary `worker_report_t` struct, and the parent reads it without disk latency.
- **Named Pipes (FIFOs)**:
  Created via `mkfifo()`. Appears as a special FIFO inode in `/tmp`. Any worker or monitor can open the FIFO for synchronized data exchange.
- **POSIX Shared Memory**:
  Created via `shm_open()` and backed by the Linux `tmpfs` virtual filesystem (`/dev/shm`). Provides zero-copy telemetry sharing where child updates are instantly readable by the parent controller.

### Module 4: Virtual Memory & Page Faults (CO4)
- **Demand Paging**:
  When allocating memory with `mmap(MAP_ANONYMOUS | MAP_PRIVATE)`, Linux updates the process virtual address space (`VmSize`), but does NOT allocate physical RAM frames (`VmRSS`). Only when memory is touched does the CPU MMU fire a **Minor Page Fault** exception, instructing the kernel to map a physical frame.
- **Copy-On-Write (COW)**:
  Upon `fork()`, the kernel marks all page table entries as read-only. Both parent and child point to identical physical frames. As demonstrated in `./stress_tool -m cow-demo`, when the child writes to memory, the MMU triggers a COW page fault, and the kernel duplicates only the modified pages.

### Module 5: File-System Operations (CO5)
- **Low-Level Syscalls vs. Buffered I/O**:
  Instead of using standard C library streams (`fopen`, `fprintf`), `stress_tool` directly invokes `open()`, `write()`, `lseek()`, and `close()`, interacting directly with the Linux Virtual File System (VFS) and process File Descriptor Table.
- **Inodes and Dentries**:
  The filesystem maps filenames to directory entries (dentries), which point to inode structures containing file metadata, block pointers, permissions, and timestamps.

### Module 6: Multithreading & Synchronization (CO6)
- **Shared Address Space vs. Process Isolation**:
  Unlike processes which have isolated address spaces, POSIX pthreads share the code segment, data segment, and heap, having only their own stack and register set.
- **Race Condition & Critical Section Protection**:
  When multiple threads concurrently perform `counter++`, the assembly instructions (`read`, `add`, `write`) interleave unpredictably across CPU cores. With `--no-sync`, lost updates reach up to 70%! Wrapping the update with `pthread_mutex_lock()` and `pthread_mutex_unlock()` guarantees atomicity and produces 100% mathematical correctness.
- **Deadlock and Starvation Avoidance**:
  1. *Lock Ordering*: All modules adhere to a strict, global locking hierarchy.
  2. *Mutual Exclusion Control*: Locks are held strictly for the minimum necessary duration (fine-grained locking).
  3. *Condition Variable Broadcast*: `pthread_cond_broadcast` wakes all waiting worker threads simultaneously, preventing thread starvation.
