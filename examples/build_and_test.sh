#!/bin/bash
# 编译和测试守护进程通信示例

set -e  # 遇到错误立即退出

PROJECT_ROOT="/home/xinyun/Infrastructure/zero_copy_framework"
cd "$PROJECT_ROOT"

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Building Daemon & Client Examples                         ║"
echo "╚══════════════════════════════════════════════════════════════╝"

# 编译选项
CXX_FLAGS="-std=c++20 -Wall -Wextra"
INCLUDES="-I./zerocp_foundationLib/posix/ipc/include \
          -I./zerocp_foundationLib/report/include \
          -I./zerocp_foundationLib/container/include \
          -I./zerocp_foundationLib/design \
          -I./zerocp_foundationLib/posix/call/include"
UDS_SOURCE="./zerocp_foundationLib/posix/ipc/source/unix_domainsocket.cpp"

# 编译守护进程
echo ""
echo "[1/2] Compiling daemon_shm_server..."
g++ $CXX_FLAGS $INCLUDES \
    examples/daemon_shm_server.cpp \
    $UDS_SOURCE \
    -o examples/daemon_shm_server

if [ $? -eq 0 ]; then
    echo "✅ daemon_shm_server compiled successfully"
else
    echo "❌ Failed to compile daemon_shm_server"
    exit 1
fi

# 编译客户端
echo ""
echo "[2/2] Compiling client_shm_query..."
g++ $CXX_FLAGS $INCLUDES \
    examples/client_shm_query.cpp \
    $UDS_SOURCE \
    -o examples/client_shm_query

if [ $? -eq 0 ]; then
    echo "✅ client_shm_query compiled successfully"
else
    echo "❌ Failed to compile client_shm_query"
    exit 1
fi

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Build Completed Successfully!                              ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "How to run:"
echo ""
echo "  Terminal 1 (Server):"
echo "    $ ./examples/daemon_shm_server"
echo ""
echo "  Terminal 2, 3, 4... (Clients):"
echo "    $ ./examples/client_shm_query"
echo ""
echo "Press Ctrl+C in server terminal to stop."
echo ""


