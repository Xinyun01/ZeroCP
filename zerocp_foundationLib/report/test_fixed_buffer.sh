#!/bin/bash

# 编译测试程序
echo "========== 编译固定缓冲区日志系统测试 =========="

g++ -std=c++17 -pthread -O2 \
    example/test_fixed_buffer.cpp \
    source/logstream.cpp \
    source/logging.cpp \
    source/log_backend.cpp \
    -I include \
    -o test_fixed_buffer

if [ $? -eq 0 ]; then
    echo "✓ 编译成功"
    echo ""
    echo "========== 运行测试 =========="
    ./test_fixed_buffer
else
    echo "✗ 编译失败"
    exit 1
fi


