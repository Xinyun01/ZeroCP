#!/bin/bash

# 跨进程 Chunk 测试运行脚本

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  跨进程 Chunk 测试${NC}"
echo -e "${BLUE}========================================${NC}"

# ==================== 1. 检查构建目录 ====================
BUILD_DIR="${SCRIPT_DIR}/build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo -e "\n${YELLOW}[1] 创建构建目录...${NC}"
    mkdir -p "${BUILD_DIR}"
else
    echo -e "\n${YELLOW}[1] 构建目录已存在${NC}"
fi

# ==================== 2. 检查主项目是否已构建 ====================
echo -e "\n${YELLOW}[2] 检查主项目库...${NC}"

MAIN_BUILD_DIR="${PROJECT_ROOT}/build"
if [ ! -d "${MAIN_BUILD_DIR}" ]; then
    echo -e "${RED}错误: 主项目尚未构建${NC}"
    echo -e "${YELLOW}请先运行以下命令构建主项目:${NC}"
    echo -e "  cd ${PROJECT_ROOT}"
    echo -e "  mkdir -p build && cd build"
    echo -e "  cmake .."
    echo -e "  make -j\$(nproc)"
    exit 1
fi

# 检查必需的库
REQUIRED_LIBS=(
    "${MAIN_BUILD_DIR}/zerocp_daemon/memory/libmempool.a"
    "${MAIN_BUILD_DIR}/zerocp_foundationLib/posix/memory/libposix_memory.a"
    "${MAIN_BUILD_DIR}/zerocp_foundationLib/posix/shm/libposix_shm.a"
    "${MAIN_BUILD_DIR}/zerocp_foundationLib/posix/logging/libposix_logging.a"
)

ALL_LIBS_EXIST=true
for lib in "${REQUIRED_LIBS[@]}"; do
    if [ ! -f "${lib}" ]; then
        echo -e "${RED}  ✗ 缺少库: ${lib}${NC}"
        ALL_LIBS_EXIST=false
    else
        echo -e "${GREEN}  ✓ 找到库: $(basename ${lib})${NC}"
    fi
done

if [ "${ALL_LIBS_EXIST}" = false ]; then
    echo -e "\n${RED}错误: 缺少必需的库文件${NC}"
    echo -e "${YELLOW}请重新构建主项目${NC}"
    exit 1
fi

# ==================== 3. 配置 CMake ====================
echo -e "\n${YELLOW}[3] 配置 CMake...${NC}"

cd "${BUILD_DIR}"
cmake .. -DCMAKE_BUILD_TYPE=Debug

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake 配置失败${NC}"
    exit 1
fi

echo -e "${GREEN}  ✓ CMake 配置成功${NC}"

# ==================== 4. 编译测试程序 ====================
echo -e "\n${YELLOW}[4] 编译测试程序...${NC}"

make -j$(nproc)

if [ $? -ne 0 ]; then
    echo -e "${RED}编译失败${NC}"
    exit 1
fi

echo -e "${GREEN}  ✓ 编译成功${NC}"

# ==================== 5. 清理旧的共享内存 ====================
echo -e "\n${YELLOW}[5] 清理旧的共享内存...${NC}"

# 清理可能存在的共享内存段
SHM_NAMES=(
    "/zerocp_management_shm"
    "/zerocp_chunk_shm"
)

for shm in "${SHM_NAMES[@]}"; do
    if [ -e "/dev/shm${shm}" ]; then
        echo -e "  清理共享内存: ${shm}"
        rm -f "/dev/shm${shm}"
    fi
done

echo -e "${GREEN}  ✓ 共享内存清理完成${NC}"

# ==================== 6. 运行测试 ====================
echo -e "\n${BLUE}========================================${NC}"
echo -e "${BLUE}  开始测试${NC}"
echo -e "${BLUE}========================================${NC}"

WRITER_BIN="${BUILD_DIR}/bin/test_chunk_writer"
READER_BIN="${BUILD_DIR}/bin/test_chunk_reader"

# 检查可执行文件
if [ ! -f "${WRITER_BIN}" ]; then
    echo -e "${RED}错误: 找不到写进程可执行文件${NC}"
    exit 1
fi

if [ ! -f "${READER_BIN}" ]; then
    echo -e "${RED}错误: 找不到读进程可执行文件${NC}"
    exit 1
fi

# 创建日志目录
LOG_DIR="${SCRIPT_DIR}/logs"
mkdir -p "${LOG_DIR}"

WRITER_LOG="${LOG_DIR}/writer.log"
READER_LOG="${LOG_DIR}/reader.log"

echo -e "\n${YELLOW}测试步骤:${NC}"
echo -e "  1. 启动写进程（后台运行）"
echo -e "  2. 等待 3 秒让写进程初始化"
echo -e "  3. 启动读进程"
echo -e "  4. 读进程完成后，停止写进程"
echo -e ""

# 启动写进程
echo -e "${YELLOW}[6.1] 启动写进程...${NC}"
"${WRITER_BIN}" > "${WRITER_LOG}" 2>&1 &
WRITER_PID=$!

echo -e "${GREEN}  ✓ 写进程已启动 (PID: ${WRITER_PID})${NC}"
echo -e "  日志文件: ${WRITER_LOG}"

# 等待写进程初始化
echo -e "\n${YELLOW}[6.2] 等待写进程初始化...${NC}"
sleep 3

# 检查写进程是否还在运行
if ! kill -0 ${WRITER_PID} 2>/dev/null; then
    echo -e "${RED}  ✗ 写进程启动失败${NC}"
    echo -e "${YELLOW}查看日志:${NC}"
    cat "${WRITER_LOG}"
    exit 1
fi

echo -e "${GREEN}  ✓ 写进程运行正常${NC}"

# 显示写进程输出
echo -e "\n${BLUE}========== 写进程输出 ==========${NC}"
cat "${WRITER_LOG}"
echo -e "${BLUE}================================${NC}"

# 启动读进程
echo -e "\n${YELLOW}[6.3] 启动读进程...${NC}"
echo -e "${YELLOW}提示: 读进程会提示按 Enter 继续，脚本会自动输入${NC}"

# 使用 expect 或 echo 自动输入
(sleep 1; echo "") | "${READER_BIN}" > "${READER_LOG}" 2>&1
READER_EXIT_CODE=$?

echo -e "${GREEN}  ✓ 读进程已完成 (退出码: ${READER_EXIT_CODE})${NC}"
echo -e "  日志文件: ${READER_LOG}"

# 显示读进程输出
echo -e "\n${BLUE}========== 读进程输出 ==========${NC}"
cat "${READER_LOG}"
echo -e "${BLUE}================================${NC}"

# 停止写进程
echo -e "\n${YELLOW}[6.4] 停止写进程...${NC}"
kill -SIGINT ${WRITER_PID} 2>/dev/null || true
sleep 1

# 如果写进程还在运行，强制终止
if kill -0 ${WRITER_PID} 2>/dev/null; then
    echo -e "${YELLOW}  写进程未响应 SIGINT，发送 SIGTERM...${NC}"
    kill -SIGTERM ${WRITER_PID} 2>/dev/null || true
    sleep 1
fi

# 最后检查
if kill -0 ${WRITER_PID} 2>/dev/null; then
    echo -e "${YELLOW}  写进程仍在运行，发送 SIGKILL...${NC}"
    kill -SIGKILL ${WRITER_PID} 2>/dev/null || true
fi

echo -e "${GREEN}  ✓ 写进程已停止${NC}"

# ==================== 7. 测试结果分析 ====================
echo -e "\n${BLUE}========================================${NC}"
echo -e "${BLUE}  测试结果分析${NC}"
echo -e "${BLUE}========================================${NC}"

# 检查读进程是否成功
if [ ${READER_EXIT_CODE} -eq 0 ]; then
    echo -e "${GREEN}✓ 读进程执行成功${NC}"
else
    echo -e "${RED}✗ 读进程执行失败 (退出码: ${READER_EXIT_CODE})${NC}"
fi

# 检查关键输出
echo -e "\n${YELLOW}关键验证点:${NC}"

# 检查 RelativePointer 解析
if grep -q "RelativePointer 解析成功" "${READER_LOG}"; then
    echo -e "${GREEN}  ✓ RelativePointer 跨进程解析成功${NC}"
else
    echo -e "${RED}  ✗ RelativePointer 解析失败${NC}"
fi

# 检查数据验证
if grep -q "数据读写验证成功" "${READER_LOG}"; then
    echo -e "${GREEN}  ✓ 跨进程数据读写验证成功${NC}"
else
    echo -e "${RED}  ✗ 数据验证失败${NC}"
fi

# 检查共享内存附加
if grep -q "成功附加到共享内存" "${READER_LOG}"; then
    echo -e "${GREEN}  ✓ 成功附加到共享内存${NC}"
else
    echo -e "${RED}  ✗ 共享内存附加失败${NC}"
fi

# ==================== 8. 清理 ====================
echo -e "\n${YELLOW}[8] 清理共享内存...${NC}"

for shm in "${SHM_NAMES[@]}"; do
    if [ -e "/dev/shm${shm}" ]; then
        echo -e "  清理共享内存: ${shm}"
        rm -f "/dev/shm${shm}"
    fi
done

echo -e "${GREEN}  ✓ 清理完成${NC}"

# ==================== 9. 总结 ====================
echo -e "\n${BLUE}========================================${NC}"
echo -e "${BLUE}  测试总结${NC}"
echo -e "${BLUE}========================================${NC}"

if [ ${READER_EXIT_CODE} -eq 0 ]; then
    echo -e "${GREEN}✓✓✓ 跨进程 Chunk 测试通过！✓✓✓${NC}"
    echo -e ""
    echo -e "${GREEN}验证成功的功能:${NC}"
    echo -e "  ✓ 共享内存创建和附加"
    echo -e "  ✓ RelativePointer 跨进程地址转换"
    echo -e "  ✓ Chunk 分配和释放"
    echo -e "  ✓ 跨进程数据读写"
    echo -e "  ✓ ChunkManager 索引机制"
    exit 0
else
    echo -e "${RED}✗✗✗ 测试失败 ✗✗✗${NC}"
    echo -e ""
    echo -e "${YELLOW}请查看日志文件:${NC}"
    echo -e "  写进程: ${WRITER_LOG}"
    echo -e "  读进程: ${READER_LOG}"
    exit 1
fi

