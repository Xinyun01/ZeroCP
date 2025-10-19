cmake_minimum_required(VERSION 3.10)
project(ZeroCopyReportTests)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 包含头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/include
)

# 公共源文件
set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/source/logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/source/logstream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zerocp_foundationLib/report/source/log_backend.cpp
)

# 1. 综合测试
add_executable(test_comprehensive 
    test_comprehensive.cpp 
    ${COMMON_SOURCES}
)
target_link_libraries(test_comprehensive pthread)

# 2. 性能测试
add_executable(test_performance 
    test_performance.cpp 
    ${COMMON_SOURCES}
)
target_link_libraries(test_performance pthread)

# 打印信息
message(STATUS "========================================")
message(STATUS "  ZeroCopy Report Test Suite")
message(STATUS "========================================")
message(STATUS "Build targets:")
message(STATUS "  - test_comprehensive  (Comprehensive functionality test)")
message(STATUS "  - test_performance    (Performance benchmark)")
message(STATUS "========================================")

