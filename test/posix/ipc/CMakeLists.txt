cmake_minimum_required(VERSION 3.16)
project(posix_ipc_tests)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

# 定义源文件根目录（相对于项目根目录）
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(FOUNDATION_LIB_ROOT "${PROJECT_ROOT}/zerocp_foundationLib")

# 包含头文件目录
include_directories(
    ${FOUNDATION_LIB_ROOT}/posix/ipc/include
    ${FOUNDATION_LIB_ROOT}/posix/posixcall/include
    ${FOUNDATION_LIB_ROOT}/report/include
    ${FOUNDATION_LIB_ROOT}/filesystem/include
    ${FOUNDATION_LIB_ROOT}/core/include
    ${FOUNDATION_LIB_ROOT}/design
)

# 收集所有需要的源文件
set(POSIX_IPC_SOURCES
    ${FOUNDATION_LIB_ROOT}/posix/ipc/source/posix_sharedmemory.cpp
    ${FOUNDATION_LIB_ROOT}/posix/ipc/source/posix_memorymap.cpp
)

set(REPORT_SOURCES
    ${FOUNDATION_LIB_ROOT}/report/source/logging.cpp
    ${FOUNDATION_LIB_ROOT}/report/source/logstream.cpp
    ${FOUNDATION_LIB_ROOT}/report/source/log_backend.cpp
    ${FOUNDATION_LIB_ROOT}/report/source/lockfree_ringbuffer.cpp
)

# 创建测试可执行文件
add_executable(test_posix_sharedmemory
    test_posix_sharedmemory.cpp
    ${POSIX_IPC_SOURCES}
    ${REPORT_SOURCES}
)

# 链接必要的库
target_link_libraries(test_posix_sharedmemory
    pthread
    rt
)

# 设置输出目录
set_target_properties(test_posix_sharedmemory PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

