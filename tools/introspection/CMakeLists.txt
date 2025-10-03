cmake_minimum_required(VERSION 3.16)
project(introspection_tool)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖包
find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSES REQUIRED ncurses)

# 包含 introspection 组件目录
include_directories(${CMAKE_SOURCE_DIR}/introspection/include)

# 创建可执行文件（使用不同的目标名称）
add_executable(introspection_tool introspection_main.cpp)

# 链接库
target_link_libraries(introspection_tool 
    PRIVATE
        introspection  # 链接到 introspection 组件库
        ${NCURSES_LIBRARIES}
        pthread
)

# 编译选项
target_compile_options(introspection_tool PRIVATE ${NCURSES_CFLAGS_OTHER})

# 设置输出文件名为 introspection
set_target_properties(introspection_tool PROPERTIES
    OUTPUT_NAME introspection
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装规则
install(TARGETS introspection_tool DESTINATION bin)

# 编译定义
target_compile_definitions(introspection_tool PRIVATE
    _GNU_SOURCE
)

# 优化选项
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(introspection_tool PRIVATE -O3 -DNDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(introspection_tool PRIVATE -g -O0)
endif()
