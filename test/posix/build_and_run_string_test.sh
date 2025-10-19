#!/bin/bash

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     ZeroCP String Test - Build and Run Script               ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}\n"

# 清理旧的构建
if [ -d "${BUILD_DIR}" ]; then
    echo -e "${YELLOW}🧹 Cleaning old build directory...${NC}"
    rm -rf "${BUILD_DIR}"
fi

# 创建构建目录
echo -e "${GREEN}📁 Creating build directory...${NC}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}" || exit 1

# 配置CMake
echo -e "\n${GREEN}⚙️  Configuring CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ CMake configuration failed!${NC}"
    exit 1
fi

# 编译
echo -e "\n${GREEN}🔨 Building test_string...${NC}"
cmake --build . --config Debug
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi

echo -e "\n${GREEN}✅ Build successful!${NC}"

# 运行测试
echo -e "\n${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                    Running String Tests                      ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}\n"

./test_string

# 检查测试结果
if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                  ✅ All Tests Completed!                     ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
else
    echo -e "\n${RED}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║                    ❌ Tests Failed!                          ║${NC}"
    echo -e "${RED}╚══════════════════════════════════════════════════════════════╝${NC}"
    exit 1
fi

