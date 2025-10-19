cmake_minimum_required(VERSION 3.16)
project(ZeroCP_String_Tests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# 包含头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/../../../zerocp_foundationLib/vocabulary/include
    ${CMAKE_SOURCE_DIR}/../../../zerocp_foundationLib/vocabulary
    ${CMAKE_SOURCE_DIR}/../../../zerocp_foundationLib/report/include
)

# 添加 report 模块的源文件
set(REPORT_SOURCES
    ${CMAKE_SOURCE_DIR}/../../../zerocp_foundationLib/report/source/logging.cpp
    ${CMAKE_SOURCE_DIR}/../../../zerocp_foundationLib/report/source/log_backend.cpp
    ${CMAKE_SOURCE_DIR}/../../../zerocp_foundationLib/report/source/logstream.cpp
    ${CMAKE_SOURCE_DIR}/../../../zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp
)

# 创建可执行文件 - String 测试
add_executable(test_string test_string.cpp ${REPORT_SOURCES})

# 创建可执行文件 - Overflow 测试
add_executable(test_overflow test_overflow.cpp ${REPORT_SOURCES})

# 链接线程库（日志系统需要）
find_package(Threads REQUIRED)
target_link_libraries(test_string Threads::Threads)
target_link_libraries(test_overflow Threads::Threads)

# 输出信息
message(STATUS "Configured ZeroCP String Tests")
message(STATUS "  - test_string: Stack-based string input/output tests")
message(STATUS "  - test_overflow: Capacity overflow tests with logging")

