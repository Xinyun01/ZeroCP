#!/bin/bash

echo "=========================================="
echo "测试客户端注册到服务端"
echo "=========================================="

cd /home/xinyun/Infrastructure/zero_copy_framework/test/zerocp_daemontest/service_description_test/build/bin

# 清理旧的 socket 文件
rm -f *.sock

echo ""
echo "1. 启动服务端..."
./diroute_daemon &
SERVER_PID=$!
echo "   服务端 PID: $SERVER_PID"

# 等待服务端启动
sleep 2

echo ""
echo "2. 运行客户端1..."
./client1 > /tmp/client1.log 2>&1 &
CLIENT1_PID=$!

echo "3. 运行客户端2..."
./client2 > /tmp/client2.log 2>&1 &
CLIENT2_PID=$!

echo "4. 运行客户端3..."
./client3 > /tmp/client3.log 2>&1 &
CLIENT3_PID=$!

# 等待客户端完成
echo ""
echo "等待客户端完成..."
wait $CLIENT1_PID
wait $CLIENT2_PID
wait $CLIENT3_PID

echo ""
echo "=========================================="
echo "客户端日志："
echo "=========================================="
echo ""
echo "--- Client 1 ---"
cat /tmp/client1.log
echo ""
echo "--- Client 2 ---"
cat /tmp/client2.log
echo ""
echo "--- Client 3 ---"
cat /tmp/client3.log

echo ""
echo "=========================================="
echo "停止服务端..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "测试完成！"
