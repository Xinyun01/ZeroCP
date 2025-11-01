cmake_minimum_required(VERSION 3.16)
project(mempool_manager_test)

# C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 设置项目根目录
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# 包含目录
include_directories(
    # MemPoolManager 相关
    ${PROJECT_ROOT}/zerocp_daemon/memory/include
    
    # 基础库
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/deital
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/posixcall/include
    ${PROJECT_ROOT}/zerocp_foundationLib/memory/include
    ${PROJECT_ROOT}/zerocp_foundationLib/concurrent/include
    ${PROJECT_ROOT}/zerocp_foundationLib/report/include
    ${PROJECT_ROOT}/zerocp_foundationLib/container/include
    ${PROJECT_ROOT}/zerocp_foundationLib/vocabulary/include
    ${PROJECT_ROOT}/zerocp_foundationLib/design
)

# 收集所有需要的源文件
set(MEMPOOL_SOURCES
    # MemPoolManager 相关
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/mempool_manager.cpp
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/mempool.cpp
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/mempool_config.cpp
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/mempool_allocator.cpp
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/posixshm_provider.cpp
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/chunk_manager.cpp
    
    # 内存管理库
    ${PROJECT_ROOT}/zerocp_foundationLib/memory/source/memory.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/memory/source/bump_allocator.cpp
    
    # POSIX 共享内存库
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/posix_sharedmemory.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/posix_memorymap.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/posix_sharedmemory_object.cpp
    
    # 并发库
    ${PROJECT_ROOT}/zerocp_foundationLib/concurrent/source/mpmclockfreelist.cpp
    
    # 日志库
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logging.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logstream.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/log_backend.cpp
)

# 测试服务端
add_executable(test_server 
    test_server.cpp
    ${MEMPOOL_SOURCES}
)
target_link_libraries(test_server
    pthread
    rt
)

# 测试客户端
add_executable(test_client 
    test_client.cpp
    ${MEMPOOL_SOURCES}
)
target_link_libraries(test_client
    pthread
    rt
)

# 设置输出目录
set_target_properties(test_server test_client
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# 安装规则（可选）
install(TARGETS test_server test_client
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "========================================")
message(STATUS "MemPool Manager Test Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Project root: ${PROJECT_ROOT}")
message(STATUS "========================================")

