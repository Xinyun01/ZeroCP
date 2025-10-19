#!/bin/bash

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Building IPC Test${NC}"
echo -e "${GREEN}========================================${NC}"

# 设置路径
IPC_DIR="/home/xinyun/Infrastructure/zero_copy_framework/zerocp_foundationLib/posix/ipc"
DESIGN_DIR="/home/xinyun/Infrastructure/zero_copy_framework/zerocp_foundationLib/design"
FILESYSTEM_DIR="/home/xinyun/Infrastructure/zero_copy_framework/zerocp_foundationLib/filesystem"
REPORT_DIR="/home/xinyun/Infrastructure/zero_copy_framework/zerocp_foundationLib/report"
CORE_DIR="/home/xinyun/Infrastructure/zero_copy_framework/zerocp_foundationLib/core"

POSIXCALL_DIR="/home/xinyun/Infrastructure/zero_copy_framework/zerocp_foundationLib/posix/posixcall"

INCLUDE_DIRS="-I${IPC_DIR}/include \
-I${FILESYSTEM_DIR}/include \
-I${POSIXCALL_DIR}/include \
-I${DESIGN_DIR} \
-I${REPORT_DIR}/include \
-I${CORE_DIR}/include"

# 编译选项
CXX_FLAGS="-std=c++23 -Wall -Wextra -g"
LIBS="-pthread -lrt"

# 创建 build 目录
mkdir -p ${IPC_DIR}/build
cd ${IPC_DIR}/build

# 编译测试程序
echo -e "\n${YELLOW}Compiling test_builder_size...${NC}"
g++ ${CXX_FLAGS} ${INCLUDE_DIRS} \
    ${IPC_DIR}/example/test_builder_size.cpp \
    ${IPC_DIR}/source/posix_sharedmemory_object.cpp \
    ${IPC_DIR}/source/posix_sharedmemory.cpp \
    ${IPC_DIR}/source/posix_memorymap.cpp \
    ${LIBS} \
    -o test_builder_size

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Compilation successful!${NC}"
    echo -e "\n${GREEN}Running test...${NC}"
    echo -e "${GREEN}========================================${NC}\n"
    ./test_builder_size
    echo -e "\n${GREEN}========================================${NC}"
else
    echo -e "${RED}❌ Compilation failed!${NC}"
    exit 1
fi

