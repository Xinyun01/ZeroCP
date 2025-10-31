# Zero Copy Framework - 顶层 CMake
cmake_minimum_required(VERSION 3.16)
project(zero_copy_framework VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 添加子目录
add_subdirectory(zerocp_core)
add_subdirectory(zerocp_introspection)
add_subdirectory(tools/introspection)
add_subdirectory(zerocp_tests)

# 可选：添加 examples（如果存在）
if(EXISTS ${CMAKE_SOURCE_DIR}/examples/CMakeLists.txt)
    add_subdirectory(examples)
endif()
