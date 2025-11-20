#!/bin/bash

set -euo pipefail

BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"

DEFAULT_TEST_SCRIPT="test_slot_index_timeout.sh"
TEST_SCRIPT="${1:-${DEFAULT_TEST_SCRIPT}}"
TEST_PATH="${SCRIPT_DIR}/${TEST_SCRIPT}"

log_step() {
    echo -e "${BLUE}[$(date '+%H:%M:%S')] $1${NC}"
}

fail() {
    echo -e "${RED}Error: $1${NC}" >&2
    exit 1
}

log_step "准备干净的构建目录"
rm -rf "${BUILD_DIR}"
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=RelWithDebInfo

log_step "开始并行编译（$(nproc) 线程）"
cmake --build "${BUILD_DIR}" --target all -j"$(nproc)"

[[ -x "${BIN_DIR}/diroute_main" ]] || fail "diroute_main 未生成"
[[ -x "${BIN_DIR}/client_0" ]] || fail "client_0 未生成，可能是构建失败"

log_step "构建完成，产物位于 ${BIN_DIR}"

[[ -x "${TEST_PATH}" ]] || fail "测试脚本不存在或不可执行：${TEST_PATH}"

log_step "执行测试脚本：${TEST_SCRIPT}"
shift || true
(
    cd "${SCRIPT_DIR}"
    "${TEST_PATH}" "$@"
)

echo -e "${GREEN}✓ 构建与测试全部完成${NC}"

