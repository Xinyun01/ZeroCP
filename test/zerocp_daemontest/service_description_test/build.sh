#!/bin/bash

set -e

echo "========================================="
echo "构建 Service Description Test"
echo "C++ 标准: C++23"
echo "参考: test/service_description_test 配置"
echo "========================================="
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "项目目录: $SCRIPT_DIR"
echo "构建目录: $BUILD_DIR"
echo ""

if [ ! -d "$BUILD_DIR" ]; then
    echo "创建构建目录..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

echo "配置 CMake..."
cmake ..

echo ""
echo "编译所有目标（并行构建）..."
make -j$(nproc)

echo ""
echo "========================================="
echo "编译完成！"
echo "========================================="
echo ""
echo "可执行文件: $BUILD_DIR/bin/"
ls -lh "$BUILD_DIR/bin/" 2>/dev/null

echo ""
echo "========================================="
echo "运行测试："
echo "========================================="
echo ""
echo "1. 启动守护进程（终端1）："
echo "   cd $BUILD_DIR/bin && ./diroute_daemon"
echo ""
echo "2. 测试单客户端（终端2）："
echo "   cd $BUILD_DIR/bin && ./ipc_client"
echo ""
echo "3. 测试3个客户端并发（终端2-4）："
echo "   cd $BUILD_DIR/bin"
echo "   ./client1 &"
echo "   ./client2 &"
echo "   ./client3 &"
echo "   wait"
echo ""
echo "========================================="
