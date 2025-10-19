# PosixCall 模块 CMakeLists
cmake_minimum_required(VERSION 3.16)
project(posix_call_test)

# 设置 C++ 标准 - 使用 C++23 获得最新特性
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../         # 包含 zerocp_foundationLib 根路径
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/include  # expected.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include             # 本地 include 目录
)

# 编译测试程序
add_executable(test_posix_call test_posix_call.cpp)
add_executable(test_system_calls test_system_calls.cpp)

# 链接库
target_link_libraries(test_posix_call pthread)
target_link_libraries(test_system_calls pthread)

# 设置编译选项
target_compile_options(test_posix_call PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(test_system_calls PRIVATE -Wall -Wextra -Wpedantic)

