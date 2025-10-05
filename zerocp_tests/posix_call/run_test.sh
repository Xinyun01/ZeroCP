#!/bin/bash

# ============================================
# ZeroCp POSIX Call Framework Test Runner
# ============================================

set -e  # 遇到错误立即退出

# 颜色定义
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 构建目录
BUILD_DIR="build"

# ============================================
# 函数定义
# ============================================

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  ZeroCp POSIX Call Test Suite${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_step() {
    echo -e "${YELLOW}▶ $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# ============================================
# 主流程
# ============================================

print_header

# 步骤 1: 创建构建目录
if [ ! -d "$BUILD_DIR" ]; then
    print_step "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# 步骤 2: 配置 CMake
print_step "Configuring CMake..."
if cmake .. -DCMAKE_BUILD_TYPE=Debug > /dev/null 2>&1; then
    print_success "CMake configuration successful"
else
    print_error "CMake configuration failed!"
    exit 1
fi

# 步骤 3: 编译
print_step "Building test executable..."
if make -j$(nproc) > /dev/null 2>&1; then
    print_success "Build successful"
else
    print_error "Build failed!"
    exit 1
fi

echo ""

# 步骤 4: 运行测试
print_step "Running tests..."
echo ""
echo "========================================"

if ./test_posix_call; then
    TEST_RESULT=0
    echo ""
    print_success "All tests passed! 🎉"
else
    TEST_RESULT=1
    echo ""
    print_error "Some tests failed!"
fi

# 返回到脚本目录
cd "$SCRIPT_DIR"

exit $TEST_RESULT

