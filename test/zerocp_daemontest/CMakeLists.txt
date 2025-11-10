cmake_minimum_required(VERSION 3.16)
project(zerocp_daemontest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/../..)

include_directories(
    ${PROJECT_ROOT}
    ${PROJECT_ROOT}/zerocp_daemon/communication/include
    ${PROJECT_ROOT}/zerocp_foundationLib/vocabulary/include
    ${PROJECT_ROOT}/zerocp_foundationLib/report/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/include
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/posixcall/include
)

add_executable(diroute_server
    service_description_test/server.cpp
    ${PROJECT_ROOT}/zerocp_daemon/communication/source/diroute.cpp
    ${PROJECT_ROOT}/zerocp_daemon/communication/source/runtime/ipc_interface_creator.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/unix_domainsocket.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/log_backend.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logstream.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logging.cpp
)
target_link_libraries(diroute_server pthread rt)

add_executable(ipc_client
    service_description_test/client.cpp
    ${PROJECT_ROOT}/zerocp_daemon/communication/source/runtime/ipc_interface_creator.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/posix/memory/source/unix_domainsocket.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/log_backend.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logstream.cpp
    ${PROJECT_ROOT}/zerocp_foundationLib/report/source/logging.cpp
)
target_link_libraries(ipc_client pthread rt)

set_target_properties(diroute_server ipc_client
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


