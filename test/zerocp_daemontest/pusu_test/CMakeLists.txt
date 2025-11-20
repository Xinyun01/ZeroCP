cmake_minimum_required(VERSION 3.16)
project(pusu_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../../..)
set(DAEMON_ROOT ${PROJECT_ROOT}/zerocp_daemon)
set(FOUNDATION_ROOT ${PROJECT_ROOT}/zerocp_foundationLib)

include_directories(
    ${PROJECT_ROOT}
    ${DAEMON_ROOT}
    ${DAEMON_ROOT}/communication/include
    ${DAEMON_ROOT}/memory/include
    ${DAEMON_ROOT}/mempool
    ${FOUNDATION_ROOT}
    ${FOUNDATION_ROOT}/vocabulary/include
    ${FOUNDATION_ROOT}/design
    ${FOUNDATION_ROOT}/report/include
    ${FOUNDATION_ROOT}/memory/include
    ${FOUNDATION_ROOT}/posix/memory/include
    ${FOUNDATION_ROOT}/posix/posixcall/include
    ${FOUNDATION_ROOT}/posix/ipc/include
    ${FOUNDATION_ROOT}/concurrent/include
)

file(GLOB_RECURSE FOUNDATION_SOURCES
    ${FOUNDATION_ROOT}/posix/memory/source/*.cpp
    ${FOUNDATION_ROOT}/posix/posixcall/source/*.cpp
    ${FOUNDATION_ROOT}/posix/ipc/source/*.cpp
    ${FOUNDATION_ROOT}/report/source/*.cpp
)

set(RUNTIME_SOURCES
    ${DAEMON_ROOT}/communication/source/runtime/ipc_interface_creator.cpp
    ${DAEMON_ROOT}/communication/source/runtime/ipc_runtime_interface.cpp
    ${DAEMON_ROOT}/communication/source/runtime/message_runtime.cpp
    ${DAEMON_ROOT}/communication/source/runtime/process_manager.cpp
)

set(MEMORY_SOURCES
    ${DAEMON_ROOT}/memory/source/chunk_manager.cpp
    ${DAEMON_ROOT}/memory/source/mempool_allocator.cpp
    ${DAEMON_ROOT}/memory/source/mempool_config.cpp
    ${DAEMON_ROOT}/memory/source/mempool_manager.cpp
    ${DAEMON_ROOT}/memory/source/mempool.cpp
    ${DAEMON_ROOT}/memory/source/posixshm_provider.cpp
    ${DAEMON_ROOT}/mempool/shared_chunk.cpp
)

set(COMMON_CLIENT_SOURCES
    ${DAEMON_ROOT}/communication/source/popo/posh_runtime.cpp
    ${RUNTIME_SOURCES}
    ${MEMORY_SOURCES}
    ${FOUNDATION_SOURCES}
)

add_executable(diroute_main
    ${DAEMON_ROOT}/application/diroute_main.cpp
    ${DAEMON_ROOT}/communication/source/diroute.cpp
    ${DAEMON_ROOT}/diroute/diroute_memory_manager.cpp
    ${COMMON_CLIENT_SOURCES}
)

add_executable(pusu_publisher
    publisher_client.cpp
    ${COMMON_CLIENT_SOURCES}
)

add_executable(pusu_subscriber
    subscriber_client.cpp
    ${COMMON_CLIENT_SOURCES}
)

foreach(target_name IN ITEMS diroute_main pusu_publisher pusu_subscriber)
    target_link_libraries(${target_name} pthread rt)
endforeach()

message(STATUS "========================================")
message(STATUS "PUSU integration test targets")
message(STATUS "  - diroute_main")
message(STATUS "  - pusu_publisher")
message(STATUS "  - pusu_subscriber")
message(STATUS "========================================")

