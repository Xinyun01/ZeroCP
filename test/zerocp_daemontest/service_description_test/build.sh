#!/usr/bin/env bash
set -euo pipefail

# Absolute paths
REPO_ROOT="/home/xinyun/Infrastructure/zero_copy_framework"
TEST_ROOT="${REPO_ROOT}/test/zerocp_daemontest"
BUILD_DIR="${TEST_ROOT}/build"

mkdir -p "${BUILD_DIR}"

cmake -S "${TEST_ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --parallel

echo "Build completed. Binaries (if build succeeded) are in: ${BUILD_DIR}"
echo "Try running:"
echo "  ${BUILD_DIR}/diroute_server"
echo "  ${BUILD_DIR}/ipc_client"


