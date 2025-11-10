#!/bin/bash

# 手动测试脚本 - 用于分别启动写进程和读进程

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ "$1" == "writer" ]; then
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  启动写进程${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    if [ ! -f "${BUILD_DIR}/bin/test_chunk_writer" ]; then
        echo -e "${YELLOW}错误: 请先编译测试程序${NC}"
        echo -e "运行: ./run_chunk_test.sh"
        exit 1
    fi
    
    "${BUILD_DIR}/bin/test_chunk_writer"
    
elif [ "$1" == "reader" ]; then
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  启动读进程${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    if [ ! -f "${BUILD_DIR}/bin/test_chunk_reader" ]; then
        echo -e "${YELLOW}错误: 请先编译测试程序${NC}"
        echo -e "运行: ./run_chunk_test.sh"
        exit 1
    fi
    
    "${BUILD_DIR}/bin/test_chunk_reader"
    
elif [ "$1" == "clean" ]; then
    echo -e "${YELLOW}清理共享内存...${NC}"
    rm -f /dev/shm/zerocp_management_shm
    rm -f /dev/shm/zerocp_chunk_shm
    echo -e "${GREEN}✓ 清理完成${NC}"
    
else
    echo "用法:"
    echo "  $0 writer   - 启动写进程"
    echo "  $0 reader   - 启动读进程"
    echo "  $0 clean    - 清理共享内存"
    echo ""
    echo "示例："
    echo "  终端1: $0 writer"
    echo "  终端2: $0 reader"
    exit 1
fi

