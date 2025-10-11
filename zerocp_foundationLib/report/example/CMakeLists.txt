cmake_minimum_required(VERSION 3.10)
project(ZeroCopyLogExamples)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 包含头文件目录
include_directories(../include)

# 公共源文件
set(COMMON_SOURCES
    ../source/lockfree_ringbuffer.cpp
    ../source/logging.cpp
    ../source/logstream.cpp
    ../source/log_backend.cpp
)

# 1. 完整示例
add_executable(complete_demo complete_demo.cpp ${COMMON_SOURCES})
target_link_libraries(complete_demo pthread)

# 2. 后端测试
add_executable(test_backend test_backend.cpp ${COMMON_SOURCES})
target_link_libraries(test_backend pthread)

# 3. 日志流测试
add_executable(test_logstream test_logstream.cpp ${COMMON_SOURCES})
target_link_libraries(test_logstream pthread)

# 4. 固定缓冲区测试
add_executable(test_fixed_buffer test_fixed_buffer.cpp ${COMMON_SOURCES})
target_link_libraries(test_fixed_buffer pthread)

# 5. 启动测试
add_executable(test_startup test_startup.cpp ${COMMON_SOURCES})
target_link_libraries(test_startup pthread)

# 6. 综合测试（新增）
add_executable(test_comprehensive test_comprehensive.cpp ${COMMON_SOURCES})
target_link_libraries(test_comprehensive pthread)

# 打印信息
message(STATUS "Build targets: complete_demo, test_backend, test_logstream, test_fixed_buffer, test_startup, test_comprehensive")
