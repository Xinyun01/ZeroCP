#!/bin/bash

# 编译 RelativePointer 示例

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Building RelativePointer Example ===${NC}"

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXAMPLE_DIR="${PROJECT_ROOT}/examples"
BUILD_DIR="${EXAMPLE_DIR}/build"

# 创建构建目录
mkdir -p "${BUILD_DIR}"

echo -e "${YELLOW}Project Root: ${PROJECT_ROOT}${NC}"
echo -e "${YELLOW}Build Directory: ${BUILD_DIR}${NC}"

# 编译命令
g++ -std=c++23 \
    -Wall -Wextra -Wpedantic \
    -I"${PROJECT_ROOT}" \
    -o "${BUILD_DIR}/relative_pointer_example" \
    "${EXAMPLE_DIR}/relative_pointer_example.cpp" \
    "${PROJECT_ROOT}/zerocp_daemon/sharememory/source/posixshm_provider.cpp" \
    -pthread \
    -lrt

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo -e "${GREEN}Executable: ${BUILD_DIR}/relative_pointer_example${NC}"
    echo ""
    echo -e "${YELLOW}=== How to Run ===${NC}"
    echo "Terminal 1 (Writer):"
    echo "  ${BUILD_DIR}/relative_pointer_example writer"
    echo ""
    echo "Terminal 2 (Reader):"
    echo "  ${BUILD_DIR}/relative_pointer_example reader"
    echo ""
    echo "Single Terminal (Cross-segment):"
    echo "  ${BUILD_DIR}/relative_pointer_example cross"
else
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi

