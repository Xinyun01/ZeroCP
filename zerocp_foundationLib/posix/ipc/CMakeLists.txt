cmake_minimum_required(VERSION 3.16)
project(ZeroCP_IPC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项
add_compile_options(-Wall -Wextra -Wpedantic)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../filesystem/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../design/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../report/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/include
)

# 收集所有源文件
file(GLOB IPC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)

# 创建 IPC 库
add_library(zerocp_ipc STATIC ${IPC_SOURCES})

# 链接必要的库
target_link_libraries(zerocp_ipc
    pthread
    rt
)

# 添加示例子目录
add_subdirectory(example)




