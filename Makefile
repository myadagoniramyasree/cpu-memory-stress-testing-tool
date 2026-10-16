# Makefile for CPU and Memory Stress Testing Tool
# OSSP Hackathon Edition - Covering CO1 to CO6

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude -pthread -D_GNU_SOURCE -std=gnu11
LDFLAGS ?= -pthread -lm -lrt

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_MAIN = stress_tool
BIN_WORKER = stress_worker

SRCS_COMMON = $(SRC_DIR)/config.c \
              $(SRC_DIR)/cli.c \
              $(SRC_DIR)/cpu_stress.c \
              $(SRC_DIR)/memory_stress.c \
              $(SRC_DIR)/process_manager.c \
              $(SRC_DIR)/thread_manager.c \
              $(SRC_DIR)/ipc.c \
              $(SRC_DIR)/monitor.c \
              $(SRC_DIR)/logger.c \
              $(SRC_DIR)/report.c

OBJS_COMMON = $(SRCS_COMMON:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJ_MAIN = $(OBJ_DIR)/main.o
OBJ_WORKER = $(OBJ_DIR)/worker_main.o

.PHONY: all clean test help directories

all: directories $(BIN_MAIN) $(BIN_WORKER)
	@echo "Build complete. Generated binaries: $(BIN_MAIN) and $(BIN_WORKER)"

directories:
	@mkdir -p $(OBJ_DIR) results

$(BIN_MAIN): $(OBJS_COMMON) $(OBJ_MAIN)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_WORKER): $(OBJ_DIR)/worker_main.o $(OBJ_DIR)/cpu_stress.o $(OBJ_DIR)/memory_stress.o $(OBJ_DIR)/ipc.o $(OBJ_DIR)/logger.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_MAIN) $(BIN_WORKER)
	@echo "Cleaned build artifacts."

test: all
	@echo "Running automated test suite..."
	@bash tests/test_config.sh
	@bash tests/test_cpu.sh
	@bash tests/test_memory.sh
	@bash tests/test_ipc.sh
	@bash tests/test_threads.sh
	@bash tests/test_reports.sh
	@echo "All tests passed successfully!"

help:
	@echo "Available Makefile targets:"
	@echo "  all      : Build main controller ($(BIN_MAIN)) and worker binary ($(BIN_WORKER))"
	@echo "  clean    : Remove build binaries and object files"
	@echo "  test     : Run the comprehensive bash test suite"
	@echo "  help     : Display this help message"
