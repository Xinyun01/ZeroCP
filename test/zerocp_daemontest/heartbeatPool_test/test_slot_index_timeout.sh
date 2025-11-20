#!/bin/bash

set -euo pipefail

# -----------------------
# 颜色配置（方便中文提示）
# -----------------------
BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# ------------------------------------
# 路径与可执行文件（统一使用绝对路径）
# ------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${BUILD_DIR}/bin"

DAEMON_BIN="${BIN_DIR}/diroute_main"
CLIENT_PREFIX="${BIN_DIR}/client_"
TOTAL_CLIENTS=10
PRIMARY_CLIENT_INDEX=5

# --------------------------------
# 日志文件与运行状态监控变量
# --------------------------------
DAEMON_LOG="${SCRIPT_DIR}/slot_index_daemon.log"
CLIENT_LOG_PREFIX="${SCRIPT_DIR}/slot_index_client_"

DAEMON_PID=""
CLIENT_PIDS=()
MONITOR_PID=""
STATUS_MONITOR_PID=""

# ------------------------------
# 公共辅助函数
# ------------------------------
wait_for_log_pattern() {
    local pattern="$1"
    local timeout="${2:-15}"
    local waited=0
    while (( waited < timeout )); do
        if grep -q "${pattern}" "${DAEMON_LOG}"; then
            return 0
        fi
        sleep 1
        ((waited++))
    done
    return 1
}

wait_for_timeout_and_report() {
    local client_name="$1"
    local timeout="${2:-15}"
    if wait_for_log_pattern "Process timeout detected: ${client_name}" "${timeout}"; then
        echo -e "${GREEN}✓ 守护进程已检测到 ${client_name} 超时${NC}"
    else
        echo -e "${RED}✗ 未在 ${timeout}s 内检测到 ${client_name} 超时${NC}"
        echo -e "${YELLOW}守护进程日志尾部如下：${NC}"
        tail -n 40 "${DAEMON_LOG}"
        exit 1
    fi
}

terminate_client_and_wait() {
    local client_index="$1"
    local client_name="Client_${client_index}"
    local pid=""

    if (( client_index < ${#CLIENT_PIDS[@]} )); then
        pid="${CLIENT_PIDS[client_index]}"
    fi

    if [[ -z "${pid}" ]]; then
        echo -e "${YELLOW}${client_name} PID 未记录，跳过终止步骤${NC}"
        return
    fi

    if ! kill -0 "${pid}" 2>/dev/null; then
        echo -e "${YELLOW}${client_name} 已退出，跳过终止步骤${NC}"
        return
    fi

    echo -e "${YELLOW}主动终止 ${client_name} 以验证回收...${NC}"
    kill -TERM "${pid}" 2>/dev/null || true
    wait_for_timeout_and_report "${client_name}" 20
}

# ----------------
# 收尾清理函数
# ----------------
cleanup() {
    echo -e "${YELLOW}开始清理测试进程...${NC}"

    if [[ -n "${MONITOR_PID}" ]] && kill -0 "${MONITOR_PID}" 2>/dev/null; then
        kill -TERM "${MONITOR_PID}" 2>/dev/null || true
    fi

    if [[ -n "${STATUS_MONITOR_PID}" ]] && kill -0 "${STATUS_MONITOR_PID}" 2>/dev/null; then
        kill -TERM "${STATUS_MONITOR_PID}" 2>/dev/null || true
    fi

    if [[ -n "${DAEMON_PID}" ]] && kill -0 "${DAEMON_PID}" 2>/dev/null; then
        kill -TERM "${DAEMON_PID}" 2>/dev/null || true
        sleep 1
        kill -9 "${DAEMON_PID}" 2>/dev/null || true
    fi

    for pid in "${CLIENT_PIDS[@]}"; do
        if kill -0 "${pid}" 2>/dev/null; then
            kill -TERM "${pid}" 2>/dev/null || true
        fi
    done

    rm -f /dev/shm/zerocp_* 2>/dev/null || true
    rm -f "${SCRIPT_DIR}"/*.sock 2>/dev/null || true

    echo -e "${GREEN}清理完成${NC}"
}

trap cleanup EXIT

# ------------------------------
# 实时监控守护进程注册数的函数
# ------------------------------
start_registration_monitor() {
    if [[ ! -f "${DAEMON_LOG}" ]]; then
        touch "${DAEMON_LOG}"
    fi

    echo -e "${BLUE}启动实时监控：守护进程当前注册的应用数量${NC}"
    tail -n +1 -F "${DAEMON_LOG}" | awk '/Total registered processes:/ {
        printf("%s 当前注册的应用进程数：%s\n", strftime("[%H:%M:%S]"), $NF);
        fflush(stdout);
    }' &
    MONITOR_PID=$!
}

# --------------------------------------
# 实时打印每个进程 PID 与存活状态的函数
# --------------------------------------
start_process_status_monitor() {
    echo -e "${BLUE}启动实时进程状态监控（包含时间与 PID）${NC}"
    {
        while true; do
            ts="$(date "+%H:%M:%S")"

            if [[ -n "${DAEMON_PID}" ]]; then
                if kill -0 "${DAEMON_PID}" 2>/dev/null; then
                    echo -e "[${ts}] 守护进程 PID=${DAEMON_PID} 状态=存活"
                else
                    echo -e "[${ts}] 守护进程 PID=${DAEMON_PID} 状态=已退出"
                fi
            fi

            if [[ ${#CLIENT_PIDS[@]} -gt 0 ]]; then
                for idx in "${!CLIENT_PIDS[@]}"; do
                    pid="${CLIENT_PIDS[$idx]}"
                    if [[ -z "${pid}" ]]; then
                        echo -e "[${ts}] client_${idx} PID=未知 状态=未启动"
                        continue
                    fi
                    if kill -0 "${pid}" 2>/dev/null; then
                        echo -e "[${ts}] client_${idx} PID=${pid} 状态=存活"
                    else
                        echo -e "[${ts}] client_${idx} PID=${pid} 状态=已退出"
                    fi
                done
            fi

            sleep 2
        done
    } &
    STATUS_MONITOR_PID=$!
}

echo -e "${BLUE}=== 插槽索引超时测试（含中文反馈）===${NC}"

if [[ "${TOTAL_CLIENTS}" -le 0 ]]; then
    echo -e "${RED}TOTAL_CLIENTS 设置非法：${TOTAL_CLIENTS}${NC}"
    exit 1
fi

if (( PRIMARY_CLIENT_INDEX < 0 || PRIMARY_CLIENT_INDEX >= TOTAL_CLIENTS )); then
    echo -e "${RED}PRIMARY_CLIENT_INDEX (${PRIMARY_CLIENT_INDEX}) 超出客户端数量${NC}"
    exit 1
fi

if [[ ! -x "${DAEMON_BIN}" ]]; then
    echo -e "${RED}缺少必要可执行文件：${DAEMON_BIN}${NC}"
    echo -e "${YELLOW}请先运行 ${SCRIPT_DIR}/build_all.sh 进行编译${NC}"
    exit 1
fi

for ((i = 0; i < TOTAL_CLIENTS; ++i)); do
    bin="${CLIENT_PREFIX}${i}"
    if [[ ! -x "${bin}" ]]; then
        echo -e "${RED}缺少必要可执行文件：${bin}${NC}"
        echo -e "${YELLOW}请先运行 ${SCRIPT_DIR}/build_all.sh 进行编译${NC}"
        exit 1
    fi
done

echo -e "${GREEN}所有可执行文件已找到，继续执行测试...${NC}"

# 重置共享内存与日志，确保测试环境干净
rm -f /dev/shm/zerocp_* 2>/dev/null || true
rm -f "${SCRIPT_DIR}"/*.sock 2>/dev/null || true
rm -f "${DAEMON_LOG}" "${CLIENT_LOG_PREFIX}"*.log 2>/dev/null || true

echo -e "${BLUE}启动 diroute 守护进程...${NC}"
"${DAEMON_BIN}" > "${DAEMON_LOG}" 2>&1 &
DAEMON_PID=$!
sleep 1

if ! kill -0 "${DAEMON_PID}" 2>/dev/null; then
    echo -e "${RED}守护进程启动失败，查看日志：${DAEMON_LOG}${NC}"
    exit 1
fi

echo -e "${GREEN}守护进程已运行 (PID: ${DAEMON_PID})${NC}"

# 开始实时监控注册数量
start_registration_monitor

for ((i = 0; i < TOTAL_CLIENTS; ++i)); do
    CLIENT_BIN="${CLIENT_PREFIX}${i}"
    CLIENT_LOG="${CLIENT_LOG_PREFIX}${i}.log"
    echo -e "${BLUE}启动 client_${i}...${NC}"
    "${CLIENT_BIN}" > "${CLIENT_LOG}" 2>&1 &
    CLIENT_PID=$!
    CLIENT_PIDS+=("${CLIENT_PID}")
    sleep 0.3
done

start_process_status_monitor

sleep 3

if [[ ${#CLIENT_PIDS[@]} -lt ${TOTAL_CLIENTS} ]]; then
    echo -e "${RED}客户端进程启动异常${NC}"
    exit 1
fi

terminate_client_and_wait "${PRIMARY_CLIENT_INDEX}"

echo -e "${BLUE}检查其余客户端是否仍然存活...${NC}"
for idx in "${!CLIENT_PIDS[@]}"; do
    if [[ "${idx}" -eq "${PRIMARY_CLIENT_INDEX}" ]]; then
        continue
    fi
    if ! kill -0 "${CLIENT_PIDS[idx]}" 2>/dev/null; then
        echo -e "${RED}Client_${idx} 意外退出${NC}"
        exit 1
    fi
done

echo -e "${YELLOW}继续按顺序终止剩余客户端并验证回收...${NC}"
for ((idx = 0; idx < TOTAL_CLIENTS; ++idx)); do
    if [[ "${idx}" -eq "${PRIMARY_CLIENT_INDEX}" ]]; then
        continue
    fi
    terminate_client_and_wait "${idx}"
done

if wait_for_log_pattern "Total registered processes: 0" 20; then
    echo -e "${GREEN}✓ 守护进程最终注册数为 0，所有槽位已释放${NC}"
else
    echo -e "${RED}✗ 未观察到守护进程注册数归零${NC}"
    tail -n 40 "${DAEMON_LOG}"
    exit 1
fi

echo -e "${BLUE}请求守护进程打印最终注册列表...${NC}"
if kill -USR1 "${DAEMON_PID}"; then
    if wait_for_log_pattern "Manual dump requested (SIGUSR1)" 10; then
        echo -e "${GREEN}✓ 已触发 Diroute::printRegisteredProcesses 手动打印${NC}"
        echo -e "${YELLOW}日志尾部输出如下：${NC}"
        tail -n 40 "${DAEMON_LOG}"
    else
        echo -e "${RED}✗ 未在期望时间内捕获手动打印日志${NC}"
        tail -n 40 "${DAEMON_LOG}"
        exit 1
    fi
else
    echo -e "${RED}✗ 无法向守护进程发送 SIGUSR1${NC}"
    exit 1
fi

echo -e "${GREEN}✓ 插槽索引超时与顺序回收逻辑验证完成${NC}"

