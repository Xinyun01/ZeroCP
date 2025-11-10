#!/bin/bash

# 编译跨进程 Chunk 测试程序
# 使用 C++23 标准和所有必要的源文件

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

echo "========================================="
echo "  编译跨进程 Chunk 测试"
echo "========================================="

# 清理旧文件
rm -f test_chunk_writer test_chunk_reader test_shared_chunk

# 包含路径
INCLUDES="-I../../zerocp_daemon/memory/include \
-I../../zerocp_daemon/mempool \
-I../../zerocp_foundationLib/posix/memory/include \
-I../../zerocp_foundationLib/posix/shm/include \
-I../../zerocp_foundationLib/posix/filesystem/include \
-I../../zerocp_foundationLib/posix/posixcall/include \
-I../../zerocp_foundationLib/container/include \
-I../../zerocp_foundationLib/design_pattern/include \
-I../../zerocp_foundationLib/vocabulary/include \
-I../../zerocp_foundationLib/concurrent/include \
-I../../zerocp_foundationLib/design \
-I../../zerocp_foundationLib/report/include \
-I../../zerocp_foundationLib/memory/include"

# 编译选项
CXXFLAGS="-std=c++23 -Wall -Wextra -g -O0 -pthread"

# 源文件
SOURCES="../../zerocp_daemon/memory/source/mempool_manager.cpp \
../../zerocp_daemon/memory/source/mempool.cpp \
../../zerocp_daemon/memory/source/mempool_allocator.cpp \
../../zerocp_daemon/memory/source/posixshm_provider.cpp \
../../zerocp_daemon/memory/source/mempool_config.cpp \
../../zerocp_daemon/memory/source/chunk_manager.cpp \
../../zerocp_daemon/mempool/shared_chunk.cpp \
../../zerocp_foundationLib/posix/memory/source/posix_sharedmemory.cpp \
../../zerocp_foundationLib/posix/memory/source/posix_sharedmemory_object.cpp \
../../zerocp_foundationLib/posix/memory/source/posix_memorymap.cpp \
../../zerocp_foundationLib/report/source/logging.cpp \
../../zerocp_foundationLib/report/source/logstream.cpp \
../../zerocp_foundationLib/report/source/log_backend.cpp \
../../zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp \
../../zerocp_foundationLib/memory/source/memory.cpp \
../../zerocp_foundationLib/memory/source/bump_allocator.cpp \
../../zerocp_foundationLib/concurrent/source/mpmclockfreelist.cpp"

# 链接库
LIBS="-lrt -lpthread"

echo ""
echo "[1] 编译 test_chunk_writer..."
g++ ${CXXFLAGS} ${INCLUDES} test_chunk_writer.cpp ${SOURCES} -o test_chunk_writer ${LIBS}

if [ $? -eq 0 ]; then
    echo "  ✓ test_chunk_writer 编译成功"
else
    echo "  ✗ test_chunk_writer 编译失败"
    exit 1
fi

echo ""
echo "[2] 编译 test_chunk_reader..."
g++ ${CXXFLAGS} ${INCLUDES} test_chunk_reader.cpp ${SOURCES} -o test_chunk_reader ${LIBS}

if [ $? -eq 0 ]; then
    echo "  ✓ test_chunk_reader 编译成功"
else
    echo "  ✗ test_chunk_reader 编译失败"
    exit 1
fi

echo ""
echo "[3] 编译 test_shared_chunk..."
g++ ${CXXFLAGS} ${INCLUDES} test_shared_chunk.cpp ${SOURCES} -o test_shared_chunk ${LIBS}

if [ $? -eq 0 ]; then
    echo "  ✓ test_shared_chunk 编译成功"
else
    echo "  ✗ test_shared_chunk 编译失败"
    exit 1
fi

echo ""
echo "========================================="
echo "  ✓ 编译完成！"
echo "========================================="
echo ""
echo "运行测试："
echo "  跨进程测试："
echo "    终端1: ./test_chunk_writer"
echo "    终端2: ./test_chunk_reader"
echo ""
echo "  SharedChunk 单元测试："
echo "    ./test_shared_chunk"
echo ""

