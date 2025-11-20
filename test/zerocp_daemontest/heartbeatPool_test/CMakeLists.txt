cmake_minimum_required(VERSION 3.16)
project(heartbeat_pool_test)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 设置路径
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../../..)
set(DAEMON_ROOT ${PROJECT_ROOT}/zerocp_daemon)
set(FOUNDATION_ROOT ${PROJECT_ROOT}/zerocp_foundationLib)
set(CORE_ROOT ${PROJECT_ROOT}/zerocp_core)

# 添加头文件路径
include_directories(
    ${PROJECT_ROOT}  # 添加根目录，使得 zerocp_foundationLib/... 能被找到
    ${DAEMON_ROOT}/communication/include
    ${DAEMON_ROOT}
    ${FOUNDATION_ROOT}
    ${FOUNDATION_ROOT}/vocabulary/include  # 为 string.inl 中的相对路径
    ${FOUNDATION_ROOT}/posix/memory/include  # 为 posix_sharedmemory_object.hpp
    ${FOUNDATION_ROOT}/posix/posixcall/include  # 为 posix_call.hpp
    ${FOUNDATION_ROOT}/posix/ipc/include  # 为 IPC 相关头文件
    ${FOUNDATION_ROOT}/report/include
    ${FOUNDATION_ROOT}/design  # 为 builder.hpp
    ${FOUNDATION_ROOT}/concurrent/include  # 为并发相关
    ${CORE_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ========================================
# 收集所有源文件
# ========================================

# Foundation库源文件
file(GLOB_RECURSE FOUNDATION_SOURCES
    ${FOUNDATION_ROOT}/posix/memory/source/*.cpp
    ${FOUNDATION_ROOT}/posix/posixcall/source/*.cpp
    ${FOUNDATION_ROOT}/posix/ipc/source/*.cpp
    ${FOUNDATION_ROOT}/report/source/*.cpp
)

# Runtime源文件
set(RUNTIME_SOURCES
    ${DAEMON_ROOT}/communication/source/runtime/ipc_interface_creator.cpp
    ${DAEMON_ROOT}/communication/source/runtime/ipc_runtime_interface.cpp
    ${DAEMON_ROOT}/communication/source/runtime/message_runtime.cpp
    ${DAEMON_ROOT}/communication/source/runtime/process_manager.cpp
)

# ========================================
# 1. 编译守护进程 (diroute_main)
# ========================================

set(DAEMON_SOURCES
    ${DAEMON_ROOT}/communication/source/diroute.cpp
    ${DAEMON_ROOT}/communication/source/popo/posh_runtime.cpp
    ${DAEMON_ROOT}/diroute/diroute_memory_manager.cpp
    ${DAEMON_ROOT}/application/diroute_main.cpp
    ${FOUNDATION_SOURCES}
    ${RUNTIME_SOURCES}
)

add_executable(diroute_main ${DAEMON_SOURCES})

target_link_libraries(diroute_main
    pthread
    rt
)

# ========================================
# 2. 编译10个独立的客户端程序
# ========================================

# 客户端共享源文件
set(CLIENT_COMMON_SOURCES
    ${DAEMON_ROOT}/communication/source/popo/posh_runtime.cpp
    ${FOUNDATION_SOURCES}
    ${RUNTIME_SOURCES}
)

foreach(i RANGE 0 9)
    add_executable(client_${i}
        client_${i}.cpp
        ${CLIENT_COMMON_SOURCES}
    )
    
    target_link_libraries(client_${i}
        pthread
        rt
    )
endforeach()

message(STATUS "Heartbeat Pool Test configured")
message(STATUS "  Daemon: diroute_main")
message(STATUS "  Clients: client_0 ~ client_9")
