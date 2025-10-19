#!/bin/bash

# 构建和运行 PosixSharedMemory 测试脚本

set -e  # 遇到错误立即退出

echo "=========================================="
echo "  Building PosixSharedMemory Tests"
echo "=========================================="

# 创建构建目录
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# 配置 CMake
echo "Configuring CMake..."
cmake .. -DCMAKE_CXX_STANDARD=23

# 编译
echo "Building..."
make test_posix_sharedmemory

echo ""
echo "=========================================="
echo "  Running Tests"
echo "=========================================="
echo ""

# 运行测试
./test_posix_sharedmemory

echo ""
echo "=========================================="
echo "  Tests Completed Successfully!"
echo "=========================================="

