#!/bin/bash

# MemPoolManager 测试运行脚本
# 包含双共享内存架构测试（管理区 + 数据区）

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${CYAN}================================================${NC}"
echo -e "${CYAN}  MemPoolManager 测试套件${NC}"
echo -e "${CYAN}  测试双共享内存架构（管理区 + 数据区）${NC}"
echo -e "${CYAN}================================================${NC}"

# 清理旧的 build 目录
if [ -d "build" ]; then
    echo -e "${YELLOW}清理旧的 build 目录...${NC}"
    rm -rf build
fi

# 创建 build 目录
mkdir -p build
cd build

# 配置 CMake
echo -e "\n${CYAN}[步骤 1/4] 配置 CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 编译
echo -e "\n${CYAN}[步骤 2/4] 编译测试程序...${NC}"
make -j$(nproc)

# 检查编译是否成功
if [ ! -f "bin/test_mempool_manager_basic" ] || [ ! -f "bin/test_mempool_manager_multiprocess" ]; then
    echo -e "${RED}❌ 编译失败！${NC}"
    exit 1
fi

echo -e "${GREEN}✓ 编译成功${NC}"

# 运行测试
echo -e "\n${CYAN}[步骤 3/4] 运行测试...${NC}"
echo ""

# 测试 1: 基础单进程测试
echo -e "${CYAN}──────────────────────────────────────────────${NC}"
echo -e "${CYAN}测试 1: 基础单进程测试${NC}"
echo -e "${CYAN}──────────────────────────────────────────────${NC}"
./bin/test_mempool_manager_basic
TEST1_RESULT=$?

echo ""

# 测试 2: 多进程共享内存测试
echo -e "${CYAN}──────────────────────────────────────────────${NC}"
echo -e "${CYAN}测试 2: 多进程共享内存测试${NC}"
echo -e "${CYAN}──────────────────────────────────────────────${NC}"
./bin/test_mempool_manager_multiprocess
TEST2_RESULT=$?

echo ""

# 总结
echo -e "\n${CYAN}[步骤 4/4] 测试总结${NC}"
echo -e "${CYAN}================================================${NC}"

if [ $TEST1_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓ 测试 1 (基础单进程): 通过${NC}"
else
    echo -e "${RED}✗ 测试 1 (基础单进程): 失败${NC}"
fi

if [ $TEST2_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓ 测试 2 (多进程共享内存): 通过${NC}"
else
    echo -e "${RED}✗ 测试 2 (多进程共享内存): 失败${NC}"
fi

echo -e "${CYAN}================================================${NC}"

# 返回总体结果
if [ $TEST1_RESULT -eq 0 ] && [ $TEST2_RESULT -eq 0 ]; then
    echo -e "${GREEN}🎉 所有测试通过！${NC}"
    exit 0
else
    echo -e "${RED}❌ 部分测试失败${NC}"
    exit 1
fi
