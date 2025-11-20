#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"

DAEMON_BIN="${BIN_DIR}/diroute_main"
PUBLISHER_BIN="${BIN_DIR}/pusu_publisher"
SUBSCRIBER_BIN="${BIN_DIR}/pusu_subscriber"

DAEMON_LOG="${SCRIPT_DIR}/daemon.log"
PUBLISHER_LOG="${SCRIPT_DIR}/publisher.log"
SUBSCRIBER_LOG="${SCRIPT_DIR}/subscriber.log"

DAEMON_PID=""
SUB_PID=""
PUB_PID=""

cleanup() {
    set +e
    if [[ -n "${PUB_PID}" ]] && kill -0 "${PUB_PID}" 2>/dev/null; then
        kill -TERM "${PUB_PID}" 2>/dev/null || true
    fi
    if [[ -n "${SUB_PID}" ]] && kill -0 "${SUB_PID}" 2>/dev/null; then
        kill -TERM "${SUB_PID}" 2>/dev/null || true
    fi
    if [[ -n "${DAEMON_PID}" ]] && kill -0 "${DAEMON_PID}" 2>/dev/null; then
        kill -TERM "${DAEMON_PID}" 2>/dev/null || true
        sleep 1
        kill -9 "${DAEMON_PID}" 2>/dev/null || true
    fi
    rm -f "${SCRIPT_DIR}"/*.sock 2>/dev/null || true
    rm -f /dev/shm/zerocp_* 2>/dev/null || true
}

trap cleanup EXIT

echo "[run] Configure & build targets..."
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=RelWithDebInfo >/dev/null
cmake --build "${BUILD_DIR}" --target diroute_main pusu_publisher pusu_subscriber -j"$(nproc)" >/dev/null

echo "[run] Cleaning residual shared memory/socket files"
rm -f "${DAEMON_LOG}" "${PUBLISHER_LOG}" "${SUBSCRIBER_LOG}"
rm -f "${SCRIPT_DIR}"/*.sock 2>/dev/null || true
rm -f /dev/shm/zerocp_* 2>/dev/null || true

echo "[run] Launching diroute_main..."
"${DAEMON_BIN}" > "${DAEMON_LOG}" 2>&1 &
DAEMON_PID=$!
sleep 2

if ! kill -0 "${DAEMON_PID}" 2>/dev/null; then
    echo "[run] diroute_main failed to start, check ${DAEMON_LOG}"
    exit 1
fi

echo "[run] Launching subscriber..."
"${SUBSCRIBER_BIN}" > "${SUBSCRIBER_LOG}" 2>&1 &
SUB_PID=$!
sleep 1

echo "[run] Launching publisher..."
"${PUBLISHER_BIN}" > "${PUBLISHER_LOG}" 2>&1 &
PUB_PID=$!

wait "${PUB_PID}"
PUBLISHER_STATUS=$?
if [[ ${PUBLISHER_STATUS} -ne 0 ]]; then
    echo "[run] Publisher failed (exit=${PUBLISHER_STATUS}). Check ${PUBLISHER_LOG}"
    exit ${PUBLISHER_STATUS}
fi

wait "${SUB_PID}"
SUBSCRIBER_STATUS=$?
if [[ ${SUBSCRIBER_STATUS} -ne 0 ]]; then
    echo "[run] Subscriber failed (exit=${SUBSCRIBER_STATUS}). Check ${SUBSCRIBER_LOG}"
    exit ${SUBSCRIBER_STATUS}
fi

echo "[run] Test completed successfully"
echo "  - Daemon log:      ${DAEMON_LOG}"
echo "  - Subscriber log:  ${SUBSCRIBER_LOG}"
echo "  - Publisher log:   ${PUBLISHER_LOG}"

