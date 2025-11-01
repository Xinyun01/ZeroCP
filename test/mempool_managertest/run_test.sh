#!/bin/bash

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  ZeroCP MemPoolManager 初始化测试${NC}"
echo -e "${GREEN}  测试目标：验证多进程共享内存${NC}"
echo -e "${GREEN}========================================${NC}"

# 1. 清理旧的共享内存和信号量
echo -e "\n${YELLOW}[1] 清理旧的共享内存和信号量...${NC}"
rm -f /dev/shm/zerocp_* 2>/dev/null
rm -f /dev/shm/sem.zerocp_init_sem 2>/dev/null
echo -e "${GREEN}  ✓ 清理完成${NC}"

# 2. 检查可执行文件
echo -e "\n${YELLOW}[2] 检查可执行文件...${NC}"
if [ ! -f "build/test_server" ]; then
    echo -e "${RED}  ✗ test_server 不存在，请先编译${NC}"
    exit 1
fi
if [ ! -f "build/test_client" ]; then
    echo -e "${RED}  ✗ test_client 不存在，请先编译${NC}"
    exit 1
fi
echo -e "${GREEN}  ✓ 可执行文件存在${NC}"

# 3. 启动服务端（后台运行）
echo -e "\n${YELLOW}[3] 启动服务端...${NC}"
./build/test_server > /tmp/test_server.log 2>&1 &
SERVER_PID=$!
echo -e "${GREEN}  ✓ 服务端已启动 (PID: $SERVER_PID)${NC}"

# 4. 等待服务端初始化
echo -e "\n${YELLOW}[4] 等待服务端初始化...${NC}"
sleep 3

# 检查服务端是否还在运行
if ! ps -p $SERVER_PID > /dev/null; then
    echo -e "${RED}  ✗ 服务端启动失败${NC}"
    echo -e "${RED}  查看日志: /tmp/test_server.log${NC}"
    cat /tmp/test_server.log
    exit 1
fi
echo -e "${GREEN}  ✓ 服务端初始化完成${NC}"

# 5. 启动客户端
echo -e "\n${YELLOW}[5] 启动客户端...${NC}"
echo -e "${GREEN}========================================${NC}"
./build/test_client
CLIENT_EXIT_CODE=$?
echo -e "${GREEN}========================================${NC}"

if [ $CLIENT_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}  ✓ 客户端执行成功${NC}"
else
    echo -e "${RED}  ✗ 客户端执行失败 (退出码: $CLIENT_EXIT_CODE)${NC}"
fi

# 6. 等待一下，让服务端输出最后的状态
echo -e "\n${YELLOW}[6] 等待服务端更新状态...${NC}"
sleep 2

# 7. 停止服务端
echo -e "\n${YELLOW}[7] 停止服务端...${NC}"
kill -SIGINT $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
echo -e "${GREEN}  ✓ 服务端已停止${NC}"

# 8. 显示服务端日志（最后 20 行）
echo -e "\n${YELLOW}[8] 服务端日志（最后 20 行）:${NC}"
echo -e "${GREEN}----------------------------------------${NC}"
tail -n 20 /tmp/test_server.log
echo -e "${GREEN}----------------------------------------${NC}"

# 9. 清理共享内存和信号量
echo -e "\n${YELLOW}[9] 清理共享内存和信号量...${NC}"
rm -f /dev/shm/zerocp_* 2>/dev/null
rm -f /dev/shm/sem.zerocp_init_sem 2>/dev/null
echo -e "${GREEN}  ✓ 清理完成${NC}"

# 10. 总结
echo -e "\n${GREEN}========================================${NC}"
if [ $CLIENT_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}  测试完成 - 成功 ✓${NC}"
else
    echo -e "${RED}  测试完成 - 失败 ✗${NC}"
fi
echo -e "${GREEN}========================================${NC}"

exit $CLIENT_EXIT_CODE

