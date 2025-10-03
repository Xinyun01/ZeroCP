#!/bin/bash
# run_tests.sh - Introspection 组件测试运行脚本

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 打印标题
print_header() {
    echo -e "${BLUE}╔════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC}  Introspection 组件测试运行器                      ${BLUE}║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}"
    echo
}

# 获取脚本所在目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build"
TEST_BINARY="${BUILD_DIR}/bin/introspection_tests"

# 解析参数
REBUILD=0
TEST_FILTER=""
VERBOSE=0
REPEAT=1
OUTPUT_XML=0
LIST_TESTS=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -r|--rebuild)
            REBUILD=1
            shift
            ;;
        -f|--filter)
            TEST_FILTER="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        --repeat)
            REPEAT="$2"
            shift 2
            ;;
        --xml)
            OUTPUT_XML=1
            shift
            ;;
        -l|--list)
            LIST_TESTS=1
            shift
            ;;
        -h|--help)
            echo "用法: $0 [选项]"
            echo
            echo "选项:"
            echo "  -r, --rebuild        重新编译测试"
            echo "  -f, --filter PATTERN 运行匹配的测试（Google Test 过滤器）"
            echo "  -v, --verbose        详细输出"
            echo "  --repeat N           重复运行 N 次"
            echo "  --xml                输出 XML 报告"
            echo "  -l, --list           列出所有测试"
            echo "  -h, --help           显示此帮助信息"
            echo
            echo "示例:"
            echo "  $0                           # 运行所有测试"
            echo "  $0 --rebuild                 # 重新编译并运行"
            echo "  $0 --filter ServerTest.*     # 只运行服务端测试"
            echo "  $0 --verbose --repeat 10     # 详细输出，重复10次"
            echo "  $0 --xml                     # 生成 XML 报告"
            exit 0
            ;;
        *)
            print_error "未知选项: $1"
            echo "使用 --help 查看帮助"
            exit 1
            ;;
    esac
done

# 打印标题
print_header

# 检查是否需要重新编译
if [ $REBUILD -eq 1 ] || [ ! -f "$TEST_BINARY" ]; then
    print_info "开始编译测试..."
    
    # 创建构建目录
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # 运行 CMake
    print_info "运行 CMake..."
    if ! cmake -DBUILD_INTROSPECTION_TESTS=ON .. ; then
        print_error "CMake 配置失败"
        exit 1
    fi
    
    # 编译
    print_info "编译测试..."
    if ! make introspection_tests -j$(nproc) ; then
        print_error "编译失败"
        exit 1
    fi
    
    print_success "编译完成"
    echo
fi

# 检查测试二进制文件是否存在
if [ ! -f "$TEST_BINARY" ]; then
    print_error "测试二进制文件不存在: $TEST_BINARY"
    print_info "请先编译测试: $0 --rebuild"
    exit 1
fi

# 列出测试
if [ $LIST_TESTS -eq 1 ]; then
    print_info "列出所有测试..."
    "$TEST_BINARY" --gtest_list_tests
    exit 0
fi

# 构建测试参数
GTEST_ARGS=""

if [ -n "$TEST_FILTER" ]; then
    GTEST_ARGS="$GTEST_ARGS --gtest_filter=$TEST_FILTER"
    print_info "测试过滤器: $TEST_FILTER"
fi

if [ $VERBOSE -eq 1 ]; then
    GTEST_ARGS="$GTEST_ARGS --gtest_print_time=1"
    print_info "详细输出模式"
fi

if [ $REPEAT -gt 1 ]; then
    GTEST_ARGS="$GTEST_ARGS --gtest_repeat=$REPEAT"
    print_info "重复运行: $REPEAT 次"
fi

if [ $OUTPUT_XML -eq 1 ]; then
    XML_OUTPUT="${BUILD_DIR}/test_results.xml"
    GTEST_ARGS="$GTEST_ARGS --gtest_output=xml:$XML_OUTPUT"
    print_info "XML 报告: $XML_OUTPUT"
fi

# 运行测试
print_info "运行测试..."
echo

cd "$BUILD_DIR"

if $TEST_BINARY $GTEST_ARGS ; then
    echo
    print_success "所有测试通过！✅"
    
    if [ $OUTPUT_XML -eq 1 ]; then
        print_info "XML 报告已保存到: $XML_OUTPUT"
    fi
    
    exit 0
else
    echo
    print_error "测试失败！❌"
    exit 1
fi

