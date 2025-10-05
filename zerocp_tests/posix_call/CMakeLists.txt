cmake_minimum_required(VERSION 3.16)
project(zerocp_posix_call_tests LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 包含头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/posix/system_call/include
)

# 添加测试可执行文件
add_executable(test_posix_call test_posix_call.cpp)
add_executable(test_posix_call_simple test_posix_call_simple.cpp)

# 设置编译选项
target_compile_options(test_posix_call PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-error
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

target_compile_options(test_posix_call_simple PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

# 启用测试
enable_testing()
add_test(
    NAME PosixCallFrameworkTest
    COMMAND test_posix_call
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_test(
    NAME PosixCallSimpleTest
    COMMAND test_posix_call_simple
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

