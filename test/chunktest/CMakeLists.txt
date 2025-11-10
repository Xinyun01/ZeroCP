# CMakeLists.txt for Chunk Cross-Process Test
cmake_minimum_required(VERSION 3.16)
project(ChunkCrossProcessTest)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 查找项目根目录
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# 设置包含目录
include_directories(
    ${PROJECT_ROOT}/zerocp_daemon/memory/include
    ${PROJECT_ROOT}/zerocp_daemon/mempool
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/shm/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/logging/include
    ${PROJECT_ROOT}/zerocp_foundationLib/container/include
)

# 查找需要链接的库
find_library(MEMPOOL_LIB NAMES mempool PATHS ${PROJECT_ROOT}/build/zerocp_daemon/memory NO_DEFAULT_PATH)
find_library(POSIX_MEMORY_LIB NAMES posix_memory PATHS ${PROJECT_ROOT}/build/zerocp_foundationLib/posix/memory NO_DEFAULT_PATH)
find_library(POSIX_SHM_LIB NAMES posix_shm PATHS ${PROJECT_ROOT}/build/zerocp_foundationLib/posix/shm NO_DEFAULT_PATH)
find_library(POSIX_LOGGING_LIB NAMES posix_logging PATHS ${PROJECT_ROOT}/build/zerocp_foundationLib/posix/logging NO_DEFAULT_PATH)

# 检查库是否找到
if(NOT MEMPOOL_LIB)
    message(FATAL_ERROR "mempool library not found. Please build the project first.")
endif()

if(NOT POSIX_MEMORY_LIB)
    message(FATAL_ERROR "posix_memory library not found. Please build the project first.")
endif()

if(NOT POSIX_SHM_LIB)
    message(FATAL_ERROR "posix_shm library not found. Please build the project first.")
endif()

if(NOT POSIX_LOGGING_LIB)
    message(FATAL_ERROR "posix_logging library not found. Please build the project first.")
endif()

message(STATUS "Found libraries:")
message(STATUS "  MEMPOOL_LIB: ${MEMPOOL_LIB}")
message(STATUS "  POSIX_MEMORY_LIB: ${POSIX_MEMORY_LIB}")
message(STATUS "  POSIX_SHM_LIB: ${POSIX_SHM_LIB}")
message(STATUS "  POSIX_LOGGING_LIB: ${POSIX_LOGGING_LIB}")

# 创建写进程可执行文件
add_executable(test_chunk_writer test_chunk_writer.cpp)
target_link_libraries(test_chunk_writer
    ${MEMPOOL_LIB}
    ${POSIX_MEMORY_LIB}
    ${POSIX_SHM_LIB}
    ${POSIX_LOGGING_LIB}
    pthread
    rt
)

# 创建读进程可执行文件
add_executable(test_chunk_reader test_chunk_reader.cpp)
target_link_libraries(test_chunk_reader
    ${MEMPOOL_LIB}
    ${POSIX_MEMORY_LIB}
    ${POSIX_SHM_LIB}
    ${POSIX_LOGGING_LIB}
    pthread
    rt
)

# 创建 SharedChunk 测试可执行文件
add_executable(test_shared_chunk test_shared_chunk.cpp)
target_link_libraries(test_shared_chunk
    ${MEMPOOL_LIB}
    ${POSIX_MEMORY_LIB}
    ${POSIX_SHM_LIB}
    ${POSIX_LOGGING_LIB}
    pthread
    rt
)

# 设置输出目录
set_target_properties(test_chunk_writer test_chunk_reader test_shared_chunk
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 打印构建信息
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  Chunk Cross-Process Test Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "========================================")

