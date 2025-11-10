cmake_minimum_required(VERSION 3.16)
project(service_description_test)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include_directories(
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/posixcall/include
    ${PROJECT_ROOT}/zerocp_foundationLib/report/include
)

set(FOUNDATION_SOURCES
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/unix_domainsocket.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logging.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logstream.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/log_backend.cpp
)

add_executable(sd_server
    server.cpp
    ${FOUNDATION_SOURCES}
)
target_link_libraries(sd_server pthread rt)

add_executable(sd_client
    client.cpp
    ${FOUNDATION_SOURCES}
)
target_link_libraries(sd_client pthread rt)

set_target_properties(sd_server sd_client
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

message(STATUS "========================================")
message(STATUS "Service Description Test")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Project root: ${PROJECT_ROOT}")
message(STATUS "========================================")


