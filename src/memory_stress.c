/**
 * @file memory_stress.c
 * @brief Memory allocation, access patterns, and virtual memory experimentation.
 * 
 * Part of CPU and Memory Stress Testing Tool for OSSP Hackathon.
 * Demonstrates CO4: Memory Stress and Virtual Memory.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include "memory_stress.h"
#include "logger.h"

void memory_touch_pages(char *buffer, size_t size, size_t page_size, mem_op_t op) {
    if (!buffer || size == 0 || page_size == 0) return;

    for (size_t offset = 0; offset < size; offset += page_size) {
        if (op == MEM_OP_READ_ONLY) {
            volatile char val = buffer[offset];
            (void)val;
        } else if (op == MEM_OP_WRITE_ONLY) {
            buffer[offset] = (char)(offset & 0xFF);
        } else { /* Read-Write */
            volatile char val = buffer[offset];
            buffer[offset] = (char)(val + 1);
        }
    }
}

void memory_access_sequential(char *buffer, size_t size, size_t page_size, mem_op_t op) {
    memory_touch_pages(buffer, size, page_size, op);
}

void memory_access_random(char *buffer, size_t size, size_t page_size, mem_op_t op, size_t touches) {
    if (!buffer || size == 0 || page_size == 0) return;

    size_t num_pages = size / page_size;
    if (num_pages == 0) return;

    for (size_t i = 0; i < touches; i++) {
        size_t page_idx = (size_t)rand() % num_pages;
        size_t offset = page_idx * page_size;

        if (op == MEM_OP_READ_ONLY) {
            volatile char val = buffer[offset];
            (void)val;
        } else if (op == MEM_OP_WRITE_ONLY) {
            buffer[offset] = (char)(i & 0xFF);
        } else {
            volatile char val = buffer[offset];
            buffer[offset] = (char)(val + 1);
        }
    }
}

int memory_stress_worker_loop(int worker_id, size_t memory_mb, int duration_sec,
                             mem_pattern_t pattern, mem_op_t op,
                             mem_alloc_type_t alloc_type,
                             shm_metrics_t *shm, worker_report_t *report) {
    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) page_size = 4096;

    size_t total_bytes = memory_mb * 1024 * 1024;
    char *buffer = NULL;

    /* Allocate memory using either malloc or mmap */
    if (alloc_type == ALLOC_MMAP) {
        buffer = (char *)mmap(NULL, total_bytes, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (buffer == MAP_FAILED) {
            log_error("Memory Error: mmap() failed for %zu MB: %s", memory_mb, strerror(errno));
            return -1;
        }
    } else {
        buffer = (char *)malloc(total_bytes);
        if (!buffer) {
            log_error("Memory Error: malloc() failed for %zu MB: %s", memory_mb, strerror(errno));
            return -1;
        }
    }

    uint64_t iterations = 0;
    uint64_t total_bytes_accessed = 0;
    pid_t my_pid = getpid();
    srand((unsigned int)(time(NULL) ^ (my_pid << 16)));

    /* Stress workload loop */
    while (!g_stop_requested) {
        if (pattern == PATTERN_SEQUENTIAL) {
            memory_access_sequential(buffer, total_bytes, (size_t)page_size, op);
            total_bytes_accessed += total_bytes;
        } else {
            /* Random access batch */
            size_t touches = (total_bytes / (size_t)page_size);
            memory_access_random(buffer, total_bytes, (size_t)page_size, op, touches);
            total_bytes_accessed += touches * (size_t)page_size;
        }
        iterations++;

        clock_gettime(CLOCK_MONOTONIC, &ts_now);
        double elapsed = (double)(ts_now.tv_sec - ts_start.tv_sec) +
                         (double)(ts_now.tv_nsec - ts_start.tv_nsec) * 1e-9;
        if (elapsed >= (double)duration_sec) {
            break;
        }

        if (shm) {
            ipc_shm_update_worker(shm, worker_id, my_pid, iterations, total_bytes_accessed);
            if (shm->terminate_flag) {
                break;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    double total_elapsed = (double)(ts_now.tv_sec - ts_start.tv_sec) +
                           (double)(ts_now.tv_nsec - ts_start.tv_nsec) * 1e-9;

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    if (report) {
        report->pid = my_pid;
        report->worker_id = worker_id;
        report->iterations = iterations;
        report->elapsed_sec = total_elapsed;
        report->user_time_sec = (double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec * 1e-6;
        report->sys_time_sec = (double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec * 1e-6;
        report->min_faults = usage.ru_minflt;
        report->maj_faults = usage.ru_majflt;
        report->memory_bytes = total_bytes;
        report->exit_code = 0;
        snprintf(report->status_msg, sizeof(report->status_msg),
                 "Worker %d processed %zu MB memory (%lu passes, %ld min faults, %ld maj faults)",
                 worker_id, memory_mb, (unsigned long)iterations, usage.ru_minflt, usage.ru_majflt);
    }

    /* Clean up allocated buffer */
    if (alloc_type == ALLOC_MMAP) {
        munmap(buffer, total_bytes);
    } else {
        free(buffer);
    }

    return 0;
}

int memory_run_cow_experiment(size_t memory_mb) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) page_size = 4096;
    size_t total_bytes = memory_mb * 1024 * 1024;
    size_t num_pages = total_bytes / (size_t)page_size;

    printf("\n=================================================================\n");
    printf("    CO4 DEMO: VIRTUAL MEMORY, DEMAND PAGING & COPY-ON-WRITE     \n");
    printf("=================================================================\n");
    printf("  Target Allocation : %zu MB (%zu pages of %ld bytes)\n", memory_mb, num_pages, page_size);

    struct rusage ru_before, ru_allocated, ru_touched, ru_child;
    getrusage(RUSAGE_SELF, &ru_before);

    printf("\n[Step 1] Allocating %zu MB virtual memory via mmap(MAP_ANONYMOUS | MAP_PRIVATE)...\n", memory_mb);
    char *buffer = (char *)mmap(NULL, total_bytes, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    getrusage(RUSAGE_SELF, &ru_allocated);
    printf("  -> Result: Virtual memory mapped. Minor page faults so far: %ld (Change: +%ld)\n",
           ru_allocated.ru_minflt, ru_allocated.ru_minflt - ru_before.ru_minflt);
    printf("  -> Concept: Demand Paging. Kernel only creates page table entries;\n"
           "              physical RAM is NOT yet committed!\n");

    printf("\n[Step 2] Touching all %zu pages (Writing 1 byte per 4096-byte page)...\n", num_pages);
    for (size_t i = 0; i < total_bytes; i += (size_t)page_size) {
        buffer[i] = (char)(i & 0xFF);
    }

    getrusage(RUSAGE_SELF, &ru_touched);
    long touch_faults = ru_touched.ru_minflt - ru_allocated.ru_minflt;
    printf("  -> Result: Minor page faults observed during touch: %ld (Expect ~%zu)\n",
           touch_faults, num_pages);
    printf("  -> Concept: Page Fault Exception. On first write, CPU MMU triggers\n"
           "              minor page fault -> Linux kernel allocates physical frame.\n");

    printf("\n[Step 3] Demonstrating Copy-On-Write (COW) via fork()...\n");
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        munmap(buffer, total_bytes);
        return -1;
    }

    if (pid == 0) {
        /* Child process */
        struct rusage child_before, child_after;
        getrusage(RUSAGE_SELF, &child_before);

        /* Read only: should NOT trigger page faults because pages are shared */
        volatile char dummy = 0;
        for (size_t i = 0; i < total_bytes; i += (size_t)page_size) {
            dummy ^= buffer[i];
        }

        struct rusage child_read;
        getrusage(RUSAGE_SELF, &child_read);
        long read_faults = child_read.ru_minflt - child_before.ru_minflt;

        /* Write to half the pages: SHOULD trigger COW page duplication */
        for (size_t i = 0; i < total_bytes / 2; i += (size_t)page_size) {
            buffer[i] = 0xAA;
        }

        getrusage(RUSAGE_SELF, &child_after);
        long cow_faults = child_after.ru_minflt - child_read.ru_minflt;

        printf("  [Child Process PID %d]\n", getpid());
        printf("    Minor faults during read pass  : %ld (Shared read-only mappings)\n", read_faults);
        printf("    Minor faults during write pass : %ld (COW duplication of %zu pages!)\n",
               cow_faults, num_pages / 2);
        printf("    Dummy accumulator              : 0x%02X\n", (unsigned char)dummy);

        munmap(buffer, total_bytes);
        exit(0);
    } else {
        /* Parent process */
        int status = 0;
        waitpid(pid, &status, 0);

        getrusage(RUSAGE_CHILDREN, &ru_child);
        printf("\n[Parent Process PID %d Verification]\n", getpid());
        printf("  Reaped child PID %d with exit status %d\n", pid, WEXITSTATUS(status));
        printf("  Cumulative child minor page faults: %ld\n", ru_child.ru_minflt);
        printf("  Conclusion: Physical memory was lazily shared until child wrote to it,\n"
               "              at which point the kernel copied only the modified frames.\n");
        printf("=================================================================\n\n");
    }

    munmap(buffer, total_bytes);
    return 0;
}
