# ==============================================================================
# Makefile for Multi-Process Unix Domain Socket Test
# ==============================================================================

# 默认目标
.DEFAULT_GOAL := all

# 颜色定义
RED    := \033[0;31m
GREEN  := \033[0;32m
YELLOW := \033[1;33m
BLUE   := \033[0;34m
NC     := \033[0m # No Color

# 构建目录
BUILD_DIR := build

# ==============================================================================
# 主要目标
# ==============================================================================

.PHONY: all
all: build test

.PHONY: help
help:
	@echo ""
	@echo "$(BLUE)Multi-Process Unix Domain Socket Test - Makefile$(NC)"
	@echo "$(BLUE)=================================================$(NC)"
	@echo ""
	@echo "Available targets:"
	@echo "  $(GREEN)make$(NC)              - Build and run tests (same as 'make all')"
	@echo "  $(GREEN)make build$(NC)        - Build the project"
	@echo "  $(GREEN)make test$(NC)         - Run tests (requires prior build)"
	@echo "  $(GREEN)make debug$(NC)        - Build in debug mode"
	@echo "  $(GREEN)make release$(NC)      - Build in release mode"
	@echo "  $(GREEN)make clean$(NC)        - Remove build directory"
	@echo "  $(GREEN)make clean-all$(NC)    - Remove build directory and socket files"
	@echo "  $(GREEN)make clean-sockets$(NC) - Remove socket files only"
	@echo "  $(GREEN)make rebuild$(NC)      - Clean and rebuild"
	@echo "  $(GREEN)make help$(NC)         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build and test in release mode"
	@echo "  make debug        # Build in debug mode"
	@echo "  make clean build  # Clean and rebuild"
	@echo ""

# ==============================================================================
# 构建目标
# ==============================================================================

.PHONY: build
build:
	@echo "$(BLUE)[BUILD]$(NC) Building project in release mode..."
	@./build.sh release
	@echo "$(GREEN)[SUCCESS]$(NC) Build completed"

.PHONY: debug
debug:
	@echo "$(BLUE)[BUILD]$(NC) Building project in debug mode..."
	@./build.sh debug
	@echo "$(GREEN)[SUCCESS]$(NC) Debug build completed"

.PHONY: release
release:
	@echo "$(BLUE)[BUILD]$(NC) Building project in release mode..."
	@./build.sh release
	@echo "$(GREEN)[SUCCESS]$(NC) Release build completed"

# ==============================================================================
# 测试目标
# ==============================================================================

.PHONY: test
test:
	@echo "$(BLUE)[TEST]$(NC) Running tests..."
	@./run_test.sh
	@echo "$(GREEN)[SUCCESS]$(NC) Tests completed"

.PHONY: run
run: test

# ==============================================================================
# 清理目标
# ==============================================================================

.PHONY: clean
clean:
	@echo "$(YELLOW)[CLEAN]$(NC) Removing build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)[SUCCESS]$(NC) Build directory removed"

.PHONY: clean-sockets
clean-sockets:
	@echo "$(YELLOW)[CLEAN]$(NC) Removing socket files..."
	@rm -f /tmp/uds_multi_process_server.sock
	@rm -f /tmp/uds_client_*.sock
	@echo "$(GREEN)[SUCCESS]$(NC) Socket files removed"

.PHONY: clean-all
clean-all: clean clean-sockets
	@echo "$(GREEN)[SUCCESS]$(NC) Full cleanup completed"

# ==============================================================================
# 重建目标
# ==============================================================================

.PHONY: rebuild
rebuild: clean build
	@echo "$(GREEN)[SUCCESS]$(NC) Rebuild completed"

.PHONY: rebuild-debug
rebuild-debug: clean debug
	@echo "$(GREEN)[SUCCESS]$(NC) Debug rebuild completed"

.PHONY: rebuild-release
rebuild-release: clean release
	@echo "$(GREEN)[SUCCESS]$(NC) Release rebuild completed"

# ==============================================================================
# 快速测试目标
# ==============================================================================

.PHONY: quick
quick:
	@echo "$(BLUE)[QUICK TEST]$(NC) Running quick build and test..."
	@./test.sh
	@echo "$(GREEN)[SUCCESS]$(NC) Quick test completed"

.PHONY: quick-debug
quick-debug:
	@echo "$(BLUE)[QUICK TEST]$(NC) Running quick debug build and test..."
	@./test.sh debug
	@echo "$(GREEN)[SUCCESS]$(NC) Quick debug test completed"

# ==============================================================================
# 实用工具目标
# ==============================================================================

.PHONY: check
check:
	@echo "$(BLUE)[CHECK]$(NC) Checking environment..."
	@command -v cmake >/dev/null 2>&1 || { echo "$(RED)[ERROR]$(NC) CMake not found"; exit 1; }
	@command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 || { echo "$(RED)[ERROR]$(NC) No C++ compiler found"; exit 1; }
	@echo "$(GREEN)[SUCCESS]$(NC) Environment check passed"

.PHONY: info
info:
	@echo ""
	@echo "$(BLUE)Project Information$(NC)"
	@echo "$(BLUE)===================$(NC)"
	@echo "Project:        Multi-Process Unix Domain Socket Test"
	@echo "Build Directory: $(BUILD_DIR)"
	@echo "C++ Standard:    C++23"
	@echo ""
	@echo "$(BLUE)Executables:$(NC)"
	@if [ -f $(BUILD_DIR)/uds_server ]; then echo "  ✓ uds_server"; else echo "  ✗ uds_server (not built)"; fi
	@if [ -f $(BUILD_DIR)/uds_client1 ]; then echo "  ✓ uds_client1"; else echo "  ✗ uds_client1 (not built)"; fi
	@if [ -f $(BUILD_DIR)/uds_client2 ]; then echo "  ✓ uds_client2"; else echo "  ✗ uds_client2 (not built)"; fi
	@if [ -f $(BUILD_DIR)/uds_client3 ]; then echo "  ✓ uds_client3"; else echo "  ✗ uds_client3 (not built)"; fi
	@echo ""

.PHONY: logs
logs:
	@echo "$(BLUE)[LOGS]$(NC) Displaying test logs..."
	@echo ""
	@echo "$(YELLOW)========== Server Log ==========$(NC)"
	@if [ -f $(BUILD_DIR)/server.log ]; then cat $(BUILD_DIR)/server.log; else echo "No server log found"; fi
	@echo ""
	@echo "$(YELLOW)========== Client 1 Log ==========$(NC)"
	@if [ -f $(BUILD_DIR)/client1.log ]; then cat $(BUILD_DIR)/client1.log; else echo "No client1 log found"; fi
	@echo ""
	@echo "$(YELLOW)========== Client 2 Log ==========$(NC)"
	@if [ -f $(BUILD_DIR)/client2.log ]; then cat $(BUILD_DIR)/client2.log; else echo "No client2 log found"; fi
	@echo ""
	@echo "$(YELLOW)========== Client 3 Log ==========$(NC)"
	@if [ -f $(BUILD_DIR)/client3.log ]; then cat $(BUILD_DIR)/client3.log; else echo "No client3 log found"; fi
	@echo ""

# ==============================================================================
# 调试目标
# ==============================================================================

.PHONY: list-sockets
list-sockets:
	@echo "$(BLUE)[INFO]$(NC) Listing Unix Domain Socket files..."
	@ls -lh /tmp/uds_*.sock 2>/dev/null || echo "No socket files found"

.PHONY: kill-processes
kill-processes:
	@echo "$(YELLOW)[KILL]$(NC) Killing all test processes..."
	@pkill -f uds_server || true
	@pkill -f uds_client || true
	@echo "$(GREEN)[SUCCESS]$(NC) All processes killed"

# ==============================================================================
# 特殊目标
# ==============================================================================

# 防止 make 将文件名当作目标
.PHONY: $(shell ls *.sh 2>/dev/null)

