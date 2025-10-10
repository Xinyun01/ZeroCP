cmake_minimum_required(VERSION 3.10)
project(LogStreamTest)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 包含头文件目录
include_directories(../include)

# 添加源文件
set(SOURCES
    ../source/logging.cpp
    ../source/logstream.cpp
    ../source/log_backend.cpp
    test_logstream.cpp
)

# 创建可执行文件
add_executable(test_logstream ${SOURCES})

# 链接线程库
target_link_libraries(test_logstream pthread)
