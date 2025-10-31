cmake_minimum_required(VERSION 3.16)
project(MemPoolManager_Test)

# 设置 C++ 标准为 C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 设置项目根目录（从 test/mempool_managertest 向上两级到达项目根目录）
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# 包含目录
include_directories(
    ${PROJECT_ROOT}/zerocp_daemon/memory/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/system_call/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/posixcall/include
    ${PROJECT_ROOT}/zerocp_foundationLib/report/include
    ${PROJECT_ROOT}/zerocp_foundationLib/memory/include
    ${PROJECT_ROOT}/zerocp_foundationLib/concurrent/include
    ${PROJECT_ROOT}/zerocp_foundationLib/vocabulary/include
    ${PROJECT_ROOT}/zerocp_foundationLib/design
)

# 打印调试信息
message(STATUS "PROJECT_ROOT = ${PROJECT_ROOT}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

# 源文件 - 内存池模块
set(MEMPOOL_SOURCES
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/mempool_manager.cpp
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/mempool_allocator.cpp
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/mempool.cpp
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/mempool_config.cpp
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/chunk_manager.cpp
)

# 源文件 - 共享内存模块
set(SHM_SOURCES
    ${PROJECT_ROOT}/zerocp_daemon/memory/source/posixshm_provider.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/posix_sharedmemory.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/posix_memorymap.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/posix_sharedmemory_object.cpp
)

# 源文件 - 日志模块
set(LOG_SOURCES
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logging.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/log_backend.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logstream.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp
)

# 源文件 - 并发模块
set(CONCURRENT_SOURCES
    ${PROJECT_ROOT}/zerocp_foundationLib/concurrent/source/mpmclockfreelist.cpp
)

# 源文件 - 内存分配器
set(MEMORY_SOURCES
    ${PROJECT_ROOT}/zerocp_foundationLib/memory/source/bump_allocator.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/memory/source/memory.cpp
)

# 所有公共源文件
set(COMMON_SOURCES
    ${MEMPOOL_SOURCES}
    ${SHM_SOURCES}
    ${LOG_SOURCES}
    ${CONCURRENT_SOURCES}
    ${MEMORY_SOURCES}
)

# ============================================================================
# 测试可执行文件 1: MemPoolManager 创建验证测试
# ============================================================================
add_executable(test_mempool_creation
    test_mempool_creation.cpp
    ${COMMON_SOURCES}
)

target_link_libraries(test_mempool_creation
    pthread
    rt  # POSIX 实时扩展（共享内存需要）
)

set_target_properties(test_mempool_creation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

# ============================================================================
# 测试可执行文件 2: 基础单进程测试
# ============================================================================
add_executable(test_mempool_manager_basic
    test_mempool_manager_basic.cpp
    ${COMMON_SOURCES}
)

target_link_libraries(test_mempool_manager_basic
    pthread
    rt  # POSIX 实时扩展（共享内存需要）
)

set_target_properties(test_mempool_manager_basic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

# ============================================================================
# 测试可执行文件 3: 多进程共享内存测试
# ============================================================================
add_executable(test_mempool_manager_multiprocess
    test_mempool_manager_multiprocess.cpp
    ${COMMON_SOURCES}
)

target_link_libraries(test_mempool_manager_multiprocess
    pthread
    rt  # POSIX 实时扩展（共享内存需要）
)

set_target_properties(test_mempool_manager_multiprocess PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

# ============================================================================
# 测试目标
# ============================================================================
enable_testing()

add_test(NAME MemPoolManager_CreationTest 
         COMMAND test_mempool_creation
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_test(NAME MemPoolManager_BasicTest 
         COMMAND test_mempool_manager_basic
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_test(NAME MemPoolManager_MultiProcessTest 
         COMMAND test_mempool_manager_multiprocess
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# ============================================================================
# 打印信息
# ============================================================================
message(STATUS "========================================")
message(STATUS "MemPoolManager Test Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Project Root: ${PROJECT_ROOT}")
message(STATUS "  Output Directory: ${CMAKE_CURRENT_BINARY_DIR}/bin")
message(STATUS "========================================")
message(STATUS "Test Executables:")
message(STATUS "  1. test_mempool_creation")
message(STATUS "  2. test_mempool_manager_basic")
message(STATUS "  3. test_mempool_manager_multiprocess")
message(STATUS "========================================")
