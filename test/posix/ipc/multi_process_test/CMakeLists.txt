cmake_minimum_required(VERSION 3.16)
project(MultiProcessUnixDomainSocketTest LANGUAGES CXX)

# ============================================================================
# 编译选项
# ============================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# ============================================================================
# 设置路径
# ============================================================================
set(FOUNDATION_LIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../zerocp_foundationLib)
set(POSIX_IPC_INCLUDE ${FOUNDATION_LIB_ROOT}/posix/ipc/include)
set(POSIX_CALL_INCLUDE ${FOUNDATION_LIB_ROOT}/posix/posixcall/include)
set(CONTAINER_INCLUDE ${FOUNDATION_LIB_ROOT}/container/include)
set(FILESYSTEM_INCLUDE ${FOUNDATION_LIB_ROOT}/filesystem/include)
set(LOGGING_INCLUDE ${FOUNDATION_LIB_ROOT}/logging/include)
set(ERROR_INCLUDE ${FOUNDATION_LIB_ROOT}/error_handling/include)
set(MEMORY_INCLUDE ${FOUNDATION_LIB_ROOT}/memory/include)
set(CONCURRENCY_INCLUDE ${FOUNDATION_LIB_ROOT}/concurrency/include)
set(PLATFORM_INCLUDE ${FOUNDATION_LIB_ROOT}/platform/include)
set(UTILITY_INCLUDE ${FOUNDATION_LIB_ROOT}/utility/include)
set(DESIGN_INCLUDE ${FOUNDATION_LIB_ROOT}/design)
set(VOCABULARY_ROOT ${FOUNDATION_LIB_ROOT}/vocabulary)
set(VOCABULARY_INCLUDE ${FOUNDATION_LIB_ROOT}/vocabulary/include)
set(REPORT_INCLUDE ${FOUNDATION_LIB_ROOT}/report/include)

# ============================================================================
# 源文件
# ============================================================================
set(POSIX_IPC_SOURCE ${FOUNDATION_LIB_ROOT}/posix/ipc/source)
set(REPORT_SOURCE ${FOUNDATION_LIB_ROOT}/report/source)
set(UDS_SOURCE_FILES
    ${POSIX_IPC_SOURCE}/unix_domainsocket.cpp
    ${REPORT_SOURCE}/logstream.cpp
    ${REPORT_SOURCE}/logging.cpp
    ${REPORT_SOURCE}/log_backend.cpp
    ${REPORT_SOURCE}/lockfree_ringbuffer.cpp
)

# ============================================================================
# 包含目录
# ============================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${POSIX_IPC_INCLUDE}
    ${POSIX_CALL_INCLUDE}
    ${CONTAINER_INCLUDE}
    ${FILESYSTEM_INCLUDE}
    ${LOGGING_INCLUDE}
    ${ERROR_INCLUDE}
    ${MEMORY_INCLUDE}
    ${CONCURRENCY_INCLUDE}
    ${PLATFORM_INCLUDE}
    ${UTILITY_INCLUDE}
    ${DESIGN_INCLUDE}
    ${VOCABULARY_ROOT}
    ${VOCABULARY_INCLUDE}
    ${REPORT_INCLUDE}
)

# ============================================================================
# 服务端可执行文件
# ============================================================================
add_executable(uds_server server.cpp ${UDS_SOURCE_FILES})
target_compile_options(uds_server PRIVATE -Wall -Wextra -Wpedantic)

# ============================================================================
# 客户端可执行文件
# ============================================================================
add_executable(uds_client1 client1.cpp ${UDS_SOURCE_FILES})
target_compile_options(uds_client1 PRIVATE -Wall -Wextra -Wpedantic)

add_executable(uds_client2 client2.cpp ${UDS_SOURCE_FILES})
target_compile_options(uds_client2 PRIVATE -Wall -Wextra -Wpedantic)

add_executable(uds_client3 client3.cpp ${UDS_SOURCE_FILES})
target_compile_options(uds_client3 PRIVATE -Wall -Wextra -Wpedantic)

add_executable(uds_client4 client4.cpp ${UDS_SOURCE_FILES})
target_compile_options(uds_client4 PRIVATE -Wall -Wextra -Wpedantic)

add_executable(uds_client5 client5.cpp ${UDS_SOURCE_FILES})
target_compile_options(uds_client5 PRIVATE -Wall -Wextra -Wpedantic)

# ============================================================================
# 安装目标（可选）
# ============================================================================
install(TARGETS uds_server uds_client1 uds_client2 uds_client3 uds_client4 uds_client5
    RUNTIME DESTINATION bin
)

# ============================================================================
# 自定义目标：清理所有 socket 文件
# ============================================================================
add_custom_target(clean_sockets
    COMMAND rm -f /tmp/uds_multi_process_server.sock
    COMMAND rm -f /tmp/uds_client_*.sock
    COMMENT "Cleaning up Unix Domain Socket files..."
)

# ============================================================================
# 自定义目标：运行测试
# ============================================================================
add_custom_target(run_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh
    DEPENDS uds_server uds_client1 uds_client2 uds_client3 uds_client4 uds_client5
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running multi-process Unix Domain Socket test..."
)

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Multi-Process UDS Test Configuration")
message(STATUS "========================================")
message(STATUS "Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "Foundation Lib:  ${FOUNDATION_LIB_ROOT}")
message(STATUS "========================================")
message(STATUS "")

