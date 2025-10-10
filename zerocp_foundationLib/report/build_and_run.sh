#!/bin/bash

# 异步日志系统编译和运行脚本

set -e  # 遇到错误立即退出

echo "╔═══════════════════════════════════════════════════════════╗"
echo "║   ZeroCopy 异步日志系统 - 编译和运行脚本                  ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# 颜色定义
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 切换到脚本所在目录
cd "$(dirname "$0")"

# 创建构建目录
echo -e "${BLUE}[1/3] 准备构建环境...${NC}"
mkdir -p build
cd build

# 编译源文件
echo -e "${BLUE}[2/3] 编译源代码...${NC}"

g++ -std=c++17 -I../include \
    -c ../source/log_backend.cpp \
    -o log_backend.o

echo "  ✓ log_backend.cpp 编译成功"

g++ -std=c++17 -I../include \
    -c ../source/logstream.cpp \
    -o logstream.o

echo "  ✓ logstream.cpp 编译成功"

g++ -std=c++17 -I../include \
    -c ../source/logging.cpp \
    -o logging.o

echo "  ✓ logging.cpp 编译成功"

# 编译完整示例
echo -e "${BLUE}[3/3] 链接可执行文件...${NC}"

g++ -std=c++17 -I../include \
    ../example/complete_demo.cpp \
    log_backend.o \
    logstream.o \
    logging.o \
    -pthread \
    -o complete_demo

echo -e "${GREEN}  ✓ complete_demo 编译成功${NC}"

# 运行示例
echo ""
echo "═══════════════════════════════════════════════════════════"
echo -e "${GREEN}编译完成！开始运行示例...${NC}"
echo "═══════════════════════════════════════════════════════════"
echo ""

./complete_demo

echo ""
echo "═══════════════════════════════════════════════════════════"
echo -e "${GREEN}✅ 程序运行完成！${NC}"
echo "═══════════════════════════════════════════════════════════"

