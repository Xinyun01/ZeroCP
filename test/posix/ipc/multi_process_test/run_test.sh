#!/bin/bash
################################################################################
# 文件：run_test.sh
# 描述：运行多进程 Unix Domain Socket 通信测试
# 用法：./run_test.sh
################################################################################

set -e  # 遇到错误立即退出

# ============================================================================
# 颜色定义
# ============================================================================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ============================================================================
# 辅助函数
# ============================================================================
info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

# ============================================================================
# 全局变量
# ============================================================================
BUILD_DIR="./build"
SERVER_PID=""
CLIENT1_PID=""
CLIENT2_PID=""
CLIENT3_PID=""
CLIENT4_PID=""
CLIENT5_PID=""
TEST_RESULT=0

# ============================================================================
# 清理函数（退出时调用）
# ============================================================================
cleanup() {
    info "Cleaning up processes and socket files..."
    
    # 杀死所有子进程
    if [ -n "$SERVER_PID" ]; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    
    if [ -n "$CLIENT1_PID" ]; then
        kill "$CLIENT1_PID" 2>/dev/null || true
        wait "$CLIENT1_PID" 2>/dev/null || true
    fi
    
    if [ -n "$CLIENT2_PID" ]; then
        kill "$CLIENT2_PID" 2>/dev/null || true
        wait "$CLIENT2_PID" 2>/dev/null || true
    fi
    
    if [ -n "$CLIENT3_PID" ]; then
        kill "$CLIENT3_PID" 2>/dev/null || true
        wait "$CLIENT3_PID" 2>/dev/null || true
    fi
    
    if [ -n "$CLIENT4_PID" ]; then
        kill "$CLIENT4_PID" 2>/dev/null || true
        wait "$CLIENT4_PID" 2>/dev/null || true
    fi
    
    if [ -n "$CLIENT5_PID" ]; then
        kill "$CLIENT5_PID" 2>/dev/null || true
        wait "$CLIENT5_PID" 2>/dev/null || true
    fi
    
    # 清理 socket 文件
    rm -f /tmp/uds_multi_process_server.sock
    rm -f /tmp/uds_client_*.sock
    
    success "Cleanup completed"
}

# 注册退出处理
trap cleanup EXIT INT TERM

# ============================================================================
# 环境检查
# ============================================================================
step "Checking test environment..."

# 检查构建目录
if [ ! -d "$BUILD_DIR" ]; then
    error "Build directory not found. Please run './build.sh' first."
    exit 1
fi

cd "$BUILD_DIR"

# 检查可执行文件
EXECUTABLES=("uds_server" "uds_client1" "uds_client2" "uds_client3" "uds_client4" "uds_client5")
for exe in "${EXECUTABLES[@]}"; do
    if [ ! -f "$exe" ]; then
        error "Executable '$exe' not found. Please run './build.sh' first."
        exit 1
    fi
    if [ ! -x "$exe" ]; then
        warning "Executable '$exe' is not executable, fixing permissions..."
        chmod +x "$exe"
    fi
done

success "All executables found"

# ============================================================================
# 清理旧的 socket 文件
# ============================================================================
step "Cleaning up old Unix Domain Socket files..."
rm -f /tmp/uds_multi_process_server.sock
rm -f /tmp/uds_client_*.sock
success "Socket files cleaned"

# ============================================================================
# 启动服务端
# ============================================================================
step "Starting server..."

./uds_server > server.log 2>&1 &
SERVER_PID=$!

# 等待服务端启动
sleep 1

# 检查服务端是否正常运行
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    error "Server failed to start. Check server.log for details."
    cat server.log
    exit 1
fi

success "Server started (PID: $SERVER_PID)"

# ============================================================================
# 启动客户端
# ============================================================================
step "Starting clients..."

# 客户端 1
./uds_client1 > client1.log 2>&1 &
CLIENT1_PID=$!
info "  Client 1 started (PID: $CLIENT1_PID)"
sleep 0.2

# 客户端 2
./uds_client2 > client2.log 2>&1 &
CLIENT2_PID=$!
info "  Client 2 started (PID: $CLIENT2_PID)"
sleep 0.2

# 客户端 3
./uds_client3 > client3.log 2>&1 &
CLIENT3_PID=$!
info "  Client 3 started (PID: $CLIENT3_PID)"
sleep 0.2

# 客户端 4
./uds_client4 > client4.log 2>&1 &
CLIENT4_PID=$!
info "  Client 4 started (PID: $CLIENT4_PID)"
sleep 0.2

# 客户端 5
./uds_client5 > client5.log 2>&1 &
CLIENT5_PID=$!
info "  Client 5 started (PID: $CLIENT5_PID)"

success "All clients started"

# ============================================================================
# 等待客户端完成
# ============================================================================
step "Waiting for clients to complete..."

# 等待客户端 1
if wait "$CLIENT1_PID"; then
    success "  ✓ Client 1 completed successfully"
else
    error "  ✗ Client 1 failed"
    TEST_RESULT=1
fi

# 等待客户端 2
if wait "$CLIENT2_PID"; then
    success "  ✓ Client 2 completed successfully"
else
    error "  ✗ Client 2 failed"
    TEST_RESULT=1
fi

# 等待客户端 3
if wait "$CLIENT3_PID"; then
    success "  ✓ Client 3 completed successfully"
else
    error "  ✗ Client 3 failed"
    TEST_RESULT=1
fi

# 等待客户端 4
if wait "$CLIENT4_PID"; then
    success "  ✓ Client 4 completed successfully"
else
    error "  ✗ Client 4 failed"
    TEST_RESULT=1
fi

# 等待客户端 5
if wait "$CLIENT5_PID"; then
    success "  ✓ Client 5 completed successfully"
else
    error "  ✗ Client 5 failed"
    TEST_RESULT=1
fi

# 标记客户端已处理
CLIENT1_PID=""
CLIENT2_PID=""
CLIENT3_PID=""
CLIENT4_PID=""
CLIENT5_PID=""

# ============================================================================
# 等待服务端完成（或超时）
# ============================================================================
step "Waiting for server to complete..."

TIMEOUT=10
ELAPSED=0

while kill -0 "$SERVER_PID" 2>/dev/null && [ $ELAPSED -lt $TIMEOUT ]; do
    sleep 1
    ELAPSED=$((ELAPSED + 1))
    echo -n "."
done
echo ""

# 如果服务端还在运行，手动停止
if kill -0 "$SERVER_PID" 2>/dev/null; then
    info "Stopping server gracefully..."
    kill -TERM "$SERVER_PID" 2>/dev/null || true
    sleep 1
    
    # 如果还在运行，强制杀死
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        warning "Server did not stop gracefully, forcing termination..."
        kill -KILL "$SERVER_PID" 2>/dev/null || true
    fi
fi

wait "$SERVER_PID" 2>/dev/null || true
SERVER_PID=""
success "Server stopped"

# ============================================================================
# 显示日志
# ============================================================================
echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Server Log${NC}"
echo -e "${CYAN}========================================${NC}"
cat server.log
echo ""

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Client 1 Log${NC}"
echo -e "${CYAN}========================================${NC}"
cat client1.log
echo ""

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Client 2 Log${NC}"
echo -e "${CYAN}========================================${NC}"
cat client2.log
echo ""

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Client 3 Log${NC}"
echo -e "${CYAN}========================================${NC}"
cat client3.log
echo ""

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Client 4 Log${NC}"
echo -e "${CYAN}========================================${NC}"
cat client4.log
echo ""

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Client 5 Log${NC}"
echo -e "${CYAN}========================================${NC}"
cat client5.log
echo ""

# ============================================================================
# 测试结果
# ============================================================================
echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Test Summary${NC}"
echo -e "${CYAN}========================================${NC}"

if [ $TEST_RESULT -eq 0 ]; then
    echo -e "  ${GREEN}✓ ALL TESTS PASSED${NC}"
    echo -e "${CYAN}========================================${NC}"
    exit 0
else
    echo -e "  ${RED}✗ SOME TESTS FAILED${NC}"
    echo -e "${CYAN}========================================${NC}"
    exit 1
fi

