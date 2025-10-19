cmake_minimum_required(VERSION 3.16)
project(ZeroCP_String_Tests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# 包含头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/../../zerocp_foundationLib/vocabulary/include
    ${CMAKE_SOURCE_DIR}/../../zerocp_foundationLib/vocabulary
)

# 创建可执行文件 - String 测试
add_executable(test_string test_string.cpp)

# 输出信息
message(STATUS "Configured ZeroCP String Tests")
message(STATUS "  - test_string: Stack-based string input/output tests")

