#!/bin/bash

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的信息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# 显示标题
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║       ZeroCP String Test - Build and Run Script             ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

print_info "当前工作目录: $SCRIPT_DIR"

# 步骤1: 清理旧的构建
print_info "步骤1: 清理旧的构建文件..."
if [ -d "build" ]; then
    rm -rf build
    print_success "清理完成"
else
    print_info "没有旧的构建文件需要清理"
fi

# 步骤2: 创建构建目录
print_info "步骤2: 创建构建目录..."
mkdir -p build
cd build
print_success "构建目录创建成功"

# 步骤3: 运行 CMake 配置
print_info "步骤3: 运行 CMake 配置..."
if cmake ..; then
    print_success "CMake 配置成功"
else
    print_error "CMake 配置失败"
    exit 1
fi

# 步骤4: 编译项目
print_info "步骤4: 编译项目..."
if make -j$(nproc); then
    print_success "编译成功"
else
    print_error "编译失败"
    exit 1
fi

# 步骤5: 运行测试
echo
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                      运行测试程序                            ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo

if [ -f "test_string" ]; then
    print_info "步骤5: 运行 test_string..."
    echo
    if ./test_string; then
        echo
        print_success "所有测试通过！"
    else
        echo
        print_error "测试执行失败"
        exit 1
    fi
else
    print_error "找不到可执行文件 test_string"
    exit 1
fi

# 完成
echo
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                  ✅ 构建和测试完成！                         ║"
echo "╚══════════════════════════════════════════════════════════════╝"

