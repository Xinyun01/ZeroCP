#!/bin/bash
################################################################################
# 文件：test.sh
# 描述：一键编译并运行多进程 Unix Domain Socket 通信测试
# 用法：./test.sh [debug|release]
################################################################################

set -e  # 遇到错误立即退出

# ============================================================================
# 颜色定义
# ============================================================================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# ============================================================================
# 辅助函数
# ============================================================================
info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

banner() {
    echo ""
    echo -e "${MAGENTA}========================================${NC}"
    echo -e "${MAGENTA}  $1${NC}"
    echo -e "${MAGENTA}========================================${NC}"
    echo ""
}

# ============================================================================
# 显示帮助信息
# ============================================================================
show_help() {
    echo "Usage: $0 [debug|release]"
    echo ""
    echo "Options:"
    echo "  debug      Build in debug mode (with debug symbols, no optimization)"
    echo "  release    Build in release mode (optimized, default)"
    echo "  -h, --help Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0           # Build and run in release mode"
    echo "  $0 debug     # Build and run in debug mode"
}

# ============================================================================
# 解析参数
# ============================================================================
BUILD_TYPE="release"

if [ $# -ge 1 ]; then
    case "$1" in
        -h|--help)
            show_help
            exit 0
            ;;
        debug|Debug|DEBUG)
            BUILD_TYPE="debug"
            ;;
        release|Release|RELEASE)
            BUILD_TYPE="release"
            ;;
        *)
            warning "Unknown option '$1', using release mode"
            ;;
    esac
fi

# ============================================================================
# 主流程
# ============================================================================
banner "Multi-Process Unix Domain Socket Test"

info "Build mode: ${BUILD_TYPE}"
echo ""

# ----------------------------------------------------------------------------
# 步骤 1: 编译
# ----------------------------------------------------------------------------
banner "Step 1: Building Project"

if ! ./build.sh "${BUILD_TYPE}"; then
    error "Build failed!"
    exit 1
fi

success "Build completed successfully"
echo ""

# ----------------------------------------------------------------------------
# 步骤 2: 运行测试
# ----------------------------------------------------------------------------
banner "Step 2: Running Test"

if ! ./run_test.sh; then
    error "Test failed!"
    exit 1
fi

success "Test completed successfully"
echo ""

# ----------------------------------------------------------------------------
# 完成
# ----------------------------------------------------------------------------
banner "Test Execution Complete"

echo -e "${GREEN}✓ All tests passed successfully!${NC}"
echo ""

exit 0

