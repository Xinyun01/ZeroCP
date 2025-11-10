#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

mkdir -p "${BUILD_DIR}"
pushd "${BUILD_DIR}" >/dev/null
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j
popd >/dev/null

# Clean old sockets
rm -f /tmp/zerocp_service_desc_server.sock
rm -f /tmp/zerocp_service_desc_client_*.sock

"${BUILD_DIR}/sd_server" &
SERVER_PID=$!
sleep 0.2

set +e
"${BUILD_DIR}/sd_client"
CLIENT_RC=$?
set -e

kill -TERM "${SERVER_PID}" >/dev/null 2>&1 || true
wait "${SERVER_PID}" 2>/dev/null || true

echo "Client exit code: ${CLIENT_RC}"
exit "${CLIENT_RC}"


