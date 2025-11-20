#!/bin/bash

# ============================================================================
# 清理脚本 - 清除所有测试残留
# ============================================================================

echo "=========================================="
echo "清理测试环境"
echo "=========================================="
echo ""

# 1. 杀死所有相关进程
echo "[1] 停止所有守护进程和客户端..."
pkill -9 diroute_main 2>/dev/null && echo "  ✓ 守护进程已停止" || echo "  - 无守护进程运行"
pkill -9 client_ 2>/dev/null && echo "  ✓ 客户端已停止" || echo "  - 无客户端运行"
sleep 1
echo ""

# 2. 删除所有 socket 文件
echo "[2] 删除 socket 文件..."
SOCK_COUNT=$(find /home/xinyun/Infrastructure/zero_copy_framework -name "*.sock" 2>/dev/null | wc -l)
if [ $SOCK_COUNT -gt 0 ]; then
    find /home/xinyun/Infrastructure/zero_copy_framework -name "*.sock" -delete
    echo "  ✓ 已删除 $SOCK_COUNT 个 socket 文件"
else
    echo "  - 无 socket 文件"
fi
echo ""

# 3. 清理共享内存
echo "[3] 清理共享内存..."
SHM_COUNT=$(ls /dev/shm/zerocp_* 2>/dev/null | wc -l)
if [ $SHM_COUNT -gt 0 ]; then
    rm -f /dev/shm/zerocp_*
    echo "  ✓ 已清理 $SHM_COUNT 个共享内存文件"
else
    echo "  - 无共享内存文件"
fi
echo ""

# 4. 清理测试日志（可选）
echo "[4] 是否清理测试日志? (y/n)"
read -r -n 1 CLEAN_LOGS
echo ""
if [ "$CLEAN_LOGS" = "y" ] || [ "$CLEAN_LOGS" = "Y" ]; then
    rm -f daemon*.log client*.log test*.log 2>/dev/null
    echo "  ✓ 测试日志已清理"
else
    echo "  - 保留测试日志"
fi
echo ""

# 5. 验证清理结果
echo "=========================================="
echo "清理完成，验证结果："
echo "=========================================="
echo ""

DAEMON_PROCS=$(pgrep -c diroute_main 2>/dev/null || echo 0)
CLIENT_PROCS=$(pgrep -c client_ 2>/dev/null || echo 0)
SOCK_FILES=$(find /home/xinyun/Infrastructure/zero_copy_framework -name "*.sock" 2>/dev/null | wc -l)
SHM_FILES=$(ls /dev/shm/zerocp_* 2>/dev/null | wc -l)

echo "守护进程: $DAEMON_PROCS"
echo "客户端进程: $CLIENT_PROCS"  
echo "Socket 文件: $SOCK_FILES"
echo "共享内存: $SHM_FILES"
echo ""

if [ $DAEMON_PROCS -eq 0 ] && [ $CLIENT_PROCS -eq 0 ] && [ $SOCK_FILES -eq 0 ] && [ $SHM_FILES -eq 0 ]; then
    echo "✅ 环境已完全清理"
else
    echo "⚠️  仍有残留，请手动检查"
fi

echo "=========================================="
