#!/bin/bash

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  POSIX IPC Test Suite Builder${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 清理旧的构建目录
if [ -d "${BUILD_DIR}" ]; then
    echo -e "${YELLOW}Cleaning old build directory...${NC}"
    rm -rf "${BUILD_DIR}"
fi

# 创建构建目录
echo -e "${GREEN}Creating build directory...${NC}"
mkdir -p "${BUILD_DIR}"

# 进入构建目录
cd "${BUILD_DIR}" || exit 1

# 运行 CMake
echo -e "${GREEN}Running CMake...${NC}"
if ! cmake ..; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}Building tests...${NC}"
if ! make; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Running Tests${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 运行测试
if [ -f "./test_posix_sharedmemory" ]; then
    ./test_posix_sharedmemory
    TEST_RESULT=$?
    
    echo ""
    if [ $TEST_RESULT -eq 0 ]; then
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}  Tests Completed Successfully!${NC}"
        echo -e "${GREEN}========================================${NC}"
    else
        echo -e "${RED}========================================${NC}"
        echo -e "${RED}  Tests Failed!${NC}"
        echo -e "${RED}========================================${NC}"
        exit $TEST_RESULT
    fi
else
    echo -e "${RED}Test executable not found!${NC}"
    exit 1
fi

