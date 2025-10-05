#!/bin/bash

# ============================================
# ZeroCp Framework - 运行所有测试
# ============================================

set -e

# 颜色定义
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly MAGENTA='\033[0;35m'
readonly NC='\033[0m'

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ============================================
# 函数定义
# ============================================

print_banner() {
    echo -e "${MAGENTA}"
    echo "╔════════════════════════════════════════╗"
    echo "║   ZeroCp Framework Test Suite         ║"
    echo "╚════════════════════════════════════════╝"
    echo -e "${NC}"
}

print_test_suite() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Running: $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

print_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✅ $2 - PASSED${NC}"
    else
        echo -e "${RED}❌ $2 - FAILED${NC}"
    fi
}

# ============================================
# 主流程
# ============================================

print_banner

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试 1: POSIX Call Framework
print_test_suite "POSIX Call Framework Tests"
if cd "$SCRIPT_DIR/posix_call" && bash run_test.sh; then
    print_result 0 "POSIX Call Framework"
    ((PASSED_TESTS++))
else
    print_result 1 "POSIX Call Framework"
    ((FAILED_TESTS++))
fi
((TOTAL_TESTS++))

cd "$SCRIPT_DIR"

# 可以在这里添加更多测试套件...
# 例如:
# print_test_suite "Another Test Suite"
# if cd "$SCRIPT_DIR/another_test" && bash run_test.sh; then
#     print_result 0 "Another Test Suite"
#     ((PASSED_TESTS++))
# else
#     print_result 1 "Another Test Suite"
#     ((FAILED_TESTS++))
# fi
# ((TOTAL_TESTS++))

# ============================================
# 打印总结
# ============================================

echo ""
echo -e "${MAGENTA}╔════════════════════════════════════════╗${NC}"
echo -e "${MAGENTA}║         Test Summary                   ║${NC}"
echo -e "${MAGENTA}╠════════════════════════════════════════╣${NC}"
echo -e "${MAGENTA}║${NC} Total Test Suites:  ${TOTAL_TESTS}                ${MAGENTA}║${NC}"
echo -e "${MAGENTA}║${NC} ${GREEN}Passed: ${PASSED_TESTS}${NC}                           ${MAGENTA}║${NC}"
echo -e "${MAGENTA}║${NC} ${RED}Failed: ${FAILED_TESTS}${NC}                           ${MAGENTA}║${NC}"
echo -e "${MAGENTA}╚════════════════════════════════════════╝${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}"
    echo "🎉 All test suites passed successfully! 🎉"
    echo -e "${NC}"
    exit 0
else
    echo -e "${RED}"
    echo "⚠️  Some test suites failed!"
    echo -e "${NC}"
    exit 1
fi

