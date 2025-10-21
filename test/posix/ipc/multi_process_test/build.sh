#!/bin/bash
################################################################################
# 文件：build.sh
# 描述：编译多进程 Unix Domain Socket 测试程序
# 用法：./build.sh [debug|release]
################################################################################

set -e  # 遇到错误立即退出

# ============================================================================
# 颜色定义
# ============================================================================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# ============================================================================
# 解析参数
# ============================================================================
BUILD_TYPE="Release"
if [ $# -ge 1 ]; then
    case "$1" in
        debug|Debug|DEBUG)
            BUILD_TYPE="Debug"
            ;;
        release|Release|RELEASE)
            BUILD_TYPE="Release"
            ;;
        *)
            warning "Unknown build type '$1', using Release"
            ;;
    esac
fi

# ============================================================================
# 环境检查
# ============================================================================
info "Checking build environment..."

# 检查 CMake
if ! command -v cmake &> /dev/null; then
    error "CMake not found. Please install CMake 3.16 or later."
    exit 1
fi

# 检查 C++ 编译器
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    error "No C++ compiler found. Please install g++ or clang++."
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
CXX_COMPILER=$(command -v g++ || command -v clang++)
info "CMake version: ${CMAKE_VERSION}"
info "C++ compiler: ${CXX_COMPILER}"

# ============================================================================
# 清理旧的 socket 文件
# ============================================================================
info "Cleaning up old Unix Domain Socket files..."
rm -f /tmp/uds_multi_process_server.sock
rm -f /tmp/uds_client_*.sock
success "Socket files cleaned"

# ============================================================================
# 构建目录
# ============================================================================
BUILD_DIR="./build"

if [ -d "$BUILD_DIR" ]; then
    warning "Build directory exists, cleaning..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
info "Created build directory: $BUILD_DIR"

# ============================================================================
# 运行 CMake
# ============================================================================
info "Running CMake configuration..."
info "Build type: ${BUILD_TYPE}"

cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" || {
    error "CMake configuration failed"
    exit 1
}

success "CMake configuration completed"

# ============================================================================
# 编译
# ============================================================================
info "Building project..."

# 检测 CPU 核心数
if command -v nproc &> /dev/null; then
    NPROC=$(nproc)
elif command -v sysctl &> /dev/null; then
    NPROC=$(sysctl -n hw.ncpu)
else
    NPROC=4
fi

info "Using ${NPROC} parallel jobs"

cmake --build . --parallel "${NPROC}" || {
    error "Build failed"
    exit 1
}

success "Build completed successfully"

# ============================================================================
# 验证可执行文件
# ============================================================================
info "Verifying executables..."

EXECUTABLES=("uds_server" "uds_client1" "uds_client2" "uds_client3")
ALL_EXIST=true

for exe in "${EXECUTABLES[@]}"; do
    if [ -f "$exe" ]; then
        success "  ✓ $exe"
    else
        error "  ✗ $exe NOT FOUND"
        ALL_EXIST=false
    fi
done

if [ "$ALL_EXIST" = false ]; then
    error "Some executables are missing"
    exit 1
fi

# ============================================================================
# 显示构建信息
# ============================================================================
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Build Summary${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "  Build Type:     ${YELLOW}${BUILD_TYPE}${NC}"
echo -e "  Build Dir:      ${YELLOW}${BUILD_DIR}${NC}"
echo -e "  Executables:    ${YELLOW}${#EXECUTABLES[@]}${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

success "All executables built successfully!"
info "Run './run_test.sh' to execute the test"

cd ..
exit 0

