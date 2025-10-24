cmake_minimum_required(VERSION 3.10)
project(ZeroCopyMemoryTests)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 包含头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/memory/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_daemon/memory/include
)

# 公共源文件 - Logging
set(LOGGING_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/source/logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/source/logstream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/source/log_backend.cpp
)

# 公共源文件 - Memory
set(MEMORY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/memory/source/memory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_daemon/memory/source/posixshm_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_daemon/memory/source/segment_allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_daemon/memory/source/pool_allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_daemon/memory/source/segmentconfig.cpp
)

# 段分配器测试
add_executable(test_segment_allocator 
    test_segment_allocator.cpp 
    ${LOGGING_SOURCES}
    ${MEMORY_SOURCES}
)
target_link_libraries(test_segment_allocator pthread rt)

# 打印信息
message(STATUS "========================================")
message(STATUS "  ZeroCopy Memory Test Suite")
message(STATUS "========================================")
message(STATUS "Build targets:")
message(STATUS "  - test_segment_allocator  (Segment allocator test)")
message(STATUS "========================================")


