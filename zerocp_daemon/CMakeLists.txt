# daemon CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(daemon VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集所有头文件
file(GLOB_RECURSE DAEMON_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

# 收集所有源文件
file(GLOB_RECURSE DAEMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# 添加守护进程可执行文件
add_executable(daemon
    ${CMAKE_CURRENT_SOURCE_DIR}/daemon_main.cpp
    ${DAEMON_SOURCES}
    ${DAEMON_HEADERS}
)

# 设置头文件包含目录
target_include_directories(daemon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 编译选项
target_compile_options(daemon PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# 输出信息
message(STATUS "Daemon 组件配置:")
message(STATUS "  - 头文件: ${DAEMON_HEADERS}")
message(STATUS "  - 源文件: ${DAEMON_SOURCES}")
message(STATUS "  - 主文件: daemon_main.cpp")
