# Introspection 组件测试

# 查找 Google Test
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(STATUS "Google Test not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# 启用测试
enable_testing()

# 测试源文件
set(TEST_SOURCES
    test_introspection_types.cpp
    test_introspection_server.cpp
    test_introspection_client.cpp
    test_integration.cpp
)

# 创建测试可执行文件
add_executable(introspection_tests ${TEST_SOURCES})

# 链接库
target_link_libraries(introspection_tests
    PRIVATE
        introspection
        GTest::gtest
        GTest::gtest_main
        pthread
)

# 包含目录
target_include_directories(introspection_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/introspection/include
)

# 添加测试
include(GoogleTest)
gtest_discover_tests(introspection_tests)

# 设置输出目录
set_target_properties(introspection_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "Introspection tests configured")

