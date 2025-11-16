cmake_minimum_required(VERSION 3.16)
project(service_description_test LANGUAGES CXX)

# 参考 test/service_description_test 的配置
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项（参考其他成功的配置）
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 项目根目录
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../../..)

# 包含目录
include_directories(
    ${PROJECT_ROOT}
    ${PROJECT_ROOT}/zerocp_daemon/communication/include
    ${PROJECT_ROOT}/zerocp_foundationLib/vocabulary/include
    ${PROJECT_ROOT}/zerocp_foundationLib/logging/include
    ${PROJECT_ROOT}/zerocp_foundationLib/report/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/posixcall/include
)

# 通用源文件列表
set(COMMON_SOURCES
    # Runtime 核心
    ${PROJECT_ROOT}/zerocp_daemon/communication/source/runtime/ipc_interface_creator.cpp
    ${PROJECT_ROOT}/zerocp_daemon/communication/source/runtime/ipc_runtime_interface.cpp
    ${PROJECT_ROOT}/zerocp_daemon/communication/source/runtime/message_runtime.cpp
    ${PROJECT_ROOT}/zerocp_daemon/communication/source/runtime/process_manager.cpp
    ${PROJECT_ROOT}/zerocp_daemon/communication/source/popo/posh_runtime.cpp
    
    # Diroute 守护进程
    ${PROJECT_ROOT}/zerocp_daemon/communication/source/diroute.cpp
    
    # POSIX 工具
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/unix_domainsocket.cpp
    
    # 日志系统
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/log_backend.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logstream.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logging.cpp
)

# ============================================================================
# 守护进程 (Server)
# ============================================================================
add_executable(diroute_daemon
    server.cpp
    ${COMMON_SOURCES}
)
target_link_libraries(diroute_daemon pthread rt)

# ============================================================================
# 单客户端测试
# ============================================================================
add_executable(ipc_client
    client.cpp
    ${COMMON_SOURCES}
)
target_link_libraries(ipc_client pthread rt)

# ============================================================================
# 多客户端并发测试
# ============================================================================

# 客户端 1
add_executable(client1
    client1.cpp
    ${COMMON_SOURCES}
)
target_link_libraries(client1 pthread rt)

# 客户端 2
add_executable(client2
    client2.cpp
    ${COMMON_SOURCES}
)
target_link_libraries(client2 pthread rt)

# 客户端 3
add_executable(client3
    client3.cpp
    ${COMMON_SOURCES}
)
target_link_libraries(client3 pthread rt)

# ============================================================================
# 输出配置
# ============================================================================
set_target_properties(
    diroute_daemon
    ipc_client
    client1
    client2
    client3
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)

# ============================================================================
# 打印构建信息
# ============================================================================
message(STATUS "========================================")
message(STATUS "Service Description Test Configuration")
message(STATUS "========================================")
message(STATUS "Build Directory: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Output Directory: ${CMAKE_CURRENT_BINARY_DIR}/bin")
message(STATUS "")
message(STATUS "Targets:")
message(STATUS "  - diroute_daemon  : Daemon server")
message(STATUS "  - ipc_client      : Single client test")
message(STATUS "  - client1         : Multi-client test 1")
message(STATUS "  - client2         : Multi-client test 2")
message(STATUS "  - client3         : Multi-client test 3")
message(STATUS "========================================")
