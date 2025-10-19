cmake_minimum_required(VERSION 3.16)
project(ZeroCP_IPC_Tests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../posixcall/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../filesystem/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../report/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../staticstl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../design
)

# Test PosixSharedMemory - 主要测试程序
add_executable(test_posix_sharedmemory 
    test_posix_sharedmemory.cpp
    ../source/posix_sharedmemory.cpp
    ../source/posix_memorymap.cpp
    ../../../report/source/logging.cpp
    ../../../report/source/logstream.cpp
    ../../../report/source/log_backend.cpp
    ../../../report/source/lockfree_ringbuffer.cpp
)
target_link_libraries(test_posix_sharedmemory 
    pthread
    rt
)

