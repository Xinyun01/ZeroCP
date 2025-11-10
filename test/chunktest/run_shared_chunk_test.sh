#!/bin/bash

# SharedChunk 测试运行脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

echo "========================================="
echo "  SharedChunk 引用计数测试"
echo "========================================="
echo ""

# 检查可执行文件是否存在
if [ ! -f test_shared_chunk ]; then
    echo "❌ test_shared_chunk 不存在，请先运行 build.sh 编译"
    exit 1
fi

echo "运行 test_shared_chunk..."
echo ""

# 运行测试
./test_shared_chunk

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo "========================================="
    echo "  ✓ 测试成功完成！"
    echo "========================================="
else
    echo "========================================="
    echo "  ✗ 测试失败 (退出码: $EXIT_CODE)"
    echo "========================================="
fi

exit $EXIT_CODE

