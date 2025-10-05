/**
 * @file test_posix_call_simple.cpp
 * @brief ZeroCp POSIX Call Framework 简单测试
 * @author ZeroCp Framework Team
 * @date 2025-10-05
 */

#include "../../zerocp_foundationLib/posix/system_call/include/posix_call.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <cstring>

using namespace ZeroCp;

const char* TEST_FILE = "/tmp/zerocp_test.txt";

// 测试 close() - 成功情况
bool test_close_success() {
    std::cout << "\n[TEST] close() - Success case" << std::endl;
    
    // 先用标准方式打开文件
    int fd = ::open(TEST_FILE, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        std::cerr << "  ❌ Failed to open file for test setup" << std::endl;
        return false;
    }
    
    std::cout << "  📁 Opened file: fd=" << fd << std::endl;
    
    // 使用框架关闭文件
    auto eval = ZeroCp_PosixCall(close)(fd)
                   .successReturnValue(0);
    
    if (!eval.hasSuccess()) {
        std::cerr << "  ❌ close() failed unexpectedly" << std::endl;
        std::cerr << "     errno=" << eval.getErrnum() << " (" << strerror(eval.getErrnum()) << ")" << std::endl;
        return false;
    }
    
    std::cout << "  ✅ close() succeeded" << std::endl;
    std::cout << "     Return value: " << eval.getValue() << std::endl;
    std::cout << "     errno: " << eval.getErrnum() << std::endl;
    
    ::unlink(TEST_FILE);
    return true;
}

// 测试 close() - 失败情况
bool test_close_failure() {
    std::cout << "\n[TEST] close() - Failure case" << std::endl;
    
    int invalid_fd = -1;
    
    // 使用框架关闭无效的 fd
    auto eval = ZeroCp_PosixCall(close)(invalid_fd)
                   .successReturnValue(0);
    
    if (eval.hasSuccess()) {
        std::cerr << "  ❌ close() should have failed but didn't" << std::endl;
        return false;
    }
    
    std::cout << "  ✅ close() failed as expected" << std::endl;
    std::cout << "     Return value: " << eval.getValue() << std::endl;
    std::cout << "     errno: " << eval.getErrnum() << " (" << strerror(eval.getErrnum()) << ")" << std::endl;
    
    return true;
}

// 测试 write() - 成功情况
bool test_write_success() {
    std::cout << "\n[TEST] write() - Success case" << std::endl;
    
    int fd = ::open(TEST_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        std::cerr << "  ❌ Failed to open file for test setup" << std::endl;
        return false;
    }
    
    const char* data = "Hello, ZeroCp!";
    size_t len = strlen(data);
    
    // 使用框架写入数据
    auto eval = ZeroCp_PosixCall(write)(fd, data, len)
                   .failureReturnValue(-1);
    
    ::close(fd);
    
    if (!eval.hasSuccess()) {
        std::cerr << "  ❌ write() failed unexpectedly" << std::endl;
        std::cerr << "     errno=" << eval.getErrnum() << " (" << strerror(eval.getErrnum()) << ")" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    ssize_t bytes_written = eval.getValue();
    std::cout << "  ✅ write() succeeded" << std::endl;
    std::cout << "     Bytes written: " << bytes_written << "/" << len << std::endl;
    
    if (static_cast<size_t>(bytes_written) != len) {
        std::cerr << "  ❌ Unexpected number of bytes written" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    ::unlink(TEST_FILE);
    return true;
}

// 测试 read() - 成功情况
bool test_read_success() {
    std::cout << "\n[TEST] read() - Success case" << std::endl;
    
    // 先写入数据
    const char* test_data = "Test Data";
    int fd_write = ::open(TEST_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    ::write(fd_write, test_data, strlen(test_data));
    ::close(fd_write);
    
    // 打开文件读取
    int fd = ::open(TEST_FILE, O_RDONLY);
    if (fd < 0) {
        std::cerr << "  ❌ Failed to open file for reading" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    char buffer[1024] = {0};
    
    // 使用框架读取数据
    auto eval = ZeroCp_PosixCall(read)(fd, buffer, sizeof(buffer))
                   .failureReturnValue(-1);
    
    ::close(fd);
    
    if (!eval.hasSuccess()) {
        std::cerr << "  ❌ read() failed unexpectedly" << std::endl;
        std::cerr << "     errno=" << eval.getErrnum() << " (" << strerror(eval.getErrnum()) << ")" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    ssize_t bytes_read = eval.getValue();
    std::cout << "  ✅ read() succeeded" << std::endl;
    std::cout << "     Bytes read: " << bytes_read << std::endl;
    std::cout << "     Data: \"" << buffer << "\"" << std::endl;
    
    if (strcmp(buffer, test_data) != 0) {
        std::cerr << "  ❌ Read data doesn't match written data" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    ::unlink(TEST_FILE);
    return true;
}

// 测试 lseek() - 成功情况
bool test_lseek_success() {
    std::cout << "\n[TEST] lseek() - Success case" << std::endl;
    
    // 创建文件并写入数据
    const char* data = "1234567890";
    int fd_write = ::open(TEST_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    ::write(fd_write, data, strlen(data));
    ::close(fd_write);
    
    // 打开文件
    int fd = ::open(TEST_FILE, O_RDONLY);
    if (fd < 0) {
        std::cerr << "  ❌ Failed to open file" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    // 使用框架定位到文件末尾
    auto eval = ZeroCp_PosixCall(lseek)(fd, 0, SEEK_END)
                   .failureReturnValue(-1);
    
    ::close(fd);
    
    if (!eval.hasSuccess()) {
        std::cerr << "  ❌ lseek() failed unexpectedly" << std::endl;
        std::cerr << "     errno=" << eval.getErrnum() << " (" << strerror(eval.getErrnum()) << ")" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    off_t file_size = eval.getValue();
    std::cout << "  ✅ lseek() succeeded" << std::endl;
    std::cout << "     File size: " << file_size << " bytes" << std::endl;
    
    if (file_size != static_cast<off_t>(strlen(data))) {
        std::cerr << "  ❌ Unexpected file size" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    ::unlink(TEST_FILE);
    return true;
}

// 测试 unlink() - 成功情况
bool test_unlink_success() {
    std::cout << "\n[TEST] unlink() - Success case" << std::endl;
    
    // 创建文件
    int fd = ::open(TEST_FILE, O_CREAT | O_WRONLY, 0644);
    ::close(fd);
    
    // 使用框架删除文件
    auto eval = ZeroCp_PosixCall(unlink)(TEST_FILE)
                   .successReturnValue(0);
    
    if (!eval.hasSuccess()) {
        std::cerr << "  ❌ unlink() failed unexpectedly" << std::endl;
        std::cerr << "     errno=" << eval.getErrnum() << " (" << strerror(eval.getErrnum()) << ")" << std::endl;
        return false;
    }
    
    std::cout << "  ✅ unlink() succeeded" << std::endl;
    
    // 验证文件已被删除
    if (access(TEST_FILE, F_OK) == 0) {
        std::cerr << "  ❌ File still exists after unlink" << std::endl;
        return false;
    }
    
    return true;
}

// 测试多个成功值
bool test_multiple_success_values() {
    std::cout << "\n[TEST] Multiple success values" << std::endl;
    
    // 创建文件
    int fd = ::open(TEST_FILE, O_CREAT | O_WRONLY, 0644);
    ::close(fd);
    
    // access() 返回 0 表示文件存在
    auto eval = ZeroCp_PosixCall(access)(TEST_FILE, F_OK)
                   .successReturnValue(0, 1, 2);  // 允许多个成功值
    
    if (!eval.hasSuccess()) {
        std::cerr << "  ❌ access() failed unexpectedly" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    std::cout << "  ✅ Multiple success values work correctly" << std::endl;
    std::cout << "     Return value: " << eval.getValue() << std::endl;
    
    ::unlink(TEST_FILE);
    return true;
}

// 测试多个失败值
bool test_multiple_failure_values() {
    std::cout << "\n[TEST] Multiple failure values" << std::endl;
    
    // 创建文件
    int fd = ::open(TEST_FILE, O_CREAT | O_WRONLY, 0644);
    ::close(fd);
    
    // access() 返回 0 表示成功，-1 表示失败
    auto eval = ZeroCp_PosixCall(access)(TEST_FILE, F_OK)
                   .failureReturnValue(-1, -2, -3);  // 排除多个失败值
    
    if (!eval.hasSuccess()) {
        std::cerr << "  ❌ access() should have succeeded" << std::endl;
        ::unlink(TEST_FILE);
        return false;
    }
    
    std::cout << "  ✅ Multiple failure values work correctly" << std::endl;
    std::cout << "     Return value: " << eval.getValue() << std::endl;
    
    ::unlink(TEST_FILE);
    return true;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  ZeroCp POSIX Call Framework Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    // 运行所有测试
    struct Test {
        const char* name;
        bool (*func)();
    };
    
    Test tests[] = {
        {"close() success", test_close_success},
        {"close() failure", test_close_failure},
        {"write() success", test_write_success},
        {"read() success", test_read_success},
        {"lseek() success", test_lseek_success},
        {"unlink() success", test_unlink_success},
        {"Multiple success values", test_multiple_success_values},
        {"Multiple failure values", test_multiple_failure_values},
    };
    
    for (const auto& test : tests) {
        if (test.func()) {
            passed++;
        } else {
            failed++;
        }
    }
    
    // 清理
    ::unlink(TEST_FILE);
    
    // 打印总结
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total:  " << (passed + failed) << std::endl;
    std::cout << "Passed: " << passed << " ✅" << std::endl;
    std::cout << "Failed: " << failed << " ❌" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return (failed == 0) ? 0 : 1;
}

