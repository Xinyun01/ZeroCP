cmake_minimum_required(VERSION 3.16)
project(introspection_component VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 源文件
set(INTROSPECTION_SOURCES
    source/introspection_server.cpp
    source/introspection_client.cpp
)

# 头文件
set(INTROSPECTION_HEADERS
    include/introspection/introspection_types.hpp
    include/introspection/introspection_server.hpp
    include/introspection/introspection_client.hpp
)

# 创建库目标
add_library(introspection STATIC ${INTROSPECTION_SOURCES} ${INTROSPECTION_HEADERS})

# 设置库的包含目录
target_include_directories(introspection
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 链接库
target_link_libraries(introspection
    PUBLIC
        pthread
)

# 编译选项
target_compile_options(introspection PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# 编译定义
target_compile_definitions(introspection PRIVATE
    _GNU_SOURCE
)

# 优化选项
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(introspection PRIVATE -O3 -DNDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(introspection PRIVATE -g -O0)
endif()

# 设置库的输出目录
set_target_properties(introspection PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# 安装规则
install(TARGETS introspection
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/introspection
    DESTINATION include
)

# 可选：导出目标供外部项目使用
# install(EXPORT introspection-targets
#     FILE introspection-targets.cmake
#     NAMESPACE zero_copy::
#     DESTINATION lib/cmake/introspection
# )

# 测试
option(BUILD_INTROSPECTION_TESTS "Build introspection tests" ON)
if(BUILD_INTROSPECTION_TESTS)
    add_subdirectory(test)
endif()

