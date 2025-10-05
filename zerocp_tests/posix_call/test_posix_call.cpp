/**
 * @file test_posix_call.cpp
 * @brief ZeroCp POSIX Call Framework 测试套件
 * @author ZeroCp Framework Team
 * @date 2025-10-05
 */

#include "../../zerocp_foundationLib/posix/system_call/include/posix_call.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <cstring>
#include <cassert>
#include <vector>
#include <string>

// 使用 ZeroCp 命名空间
using namespace ZeroCp;

// ==================== 测试辅助工具 ====================

class TestRunner {
public:
    struct TestResult {
        std::string name;
        bool passed;
        std::string message;
    };

    TestRunner(const std::string& suite_name) : m_suite_name(suite_name) {}

    void addTest(const std::string& name, bool (*func)()) {
        m_tests.push_back({name, func});
    }

    int runAll() {
        printHeader();
        
        for (const auto& test : m_tests) {
            std::cout << "\n[TEST] " << test.name << std::endl;
            
            bool passed = false;
            std::string message;
            
            try {
                passed = test.func();
                if (passed) {
                    message = "PASSED";
                } else {
                    message = "FAILED";
                }
            } catch (const std::exception& e) {
                passed = false;
                message = std::string("EXCEPTION: ") + e.what();
            }
            
            TestResult result{test.name, passed, message};
            m_results.push_back(result);
            
            if (passed) {
                std::cout << "✅ " << test.name << " - " << message << std::endl;
            } else {
                std::cout << "❌ " << test.name << " - " << message << std::endl;
            }
        }
        
        printSummary();
        return getFailedCount() == 0 ? 0 : 1;
    }

private:
    struct Test {
        std::string name;
        bool (*func)();
    };

    void printHeader() {
        std::cout << "========================================" << std::endl;
        std::cout << "  " << m_suite_name << std::endl;
        std::cout << "========================================" << std::endl;
    }

    void printSummary() {
        int passed = getPassedCount();
        int failed = getFailedCount();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Test Summary" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total:  " << (passed + failed) << std::endl;
        std::cout << "Passed: " << passed << " ✅" << std::endl;
        std::cout << "Failed: " << failed << " ❌" << std::endl;
        std::cout << "========================================" << std::endl;
    }

    int getPassedCount() const {
        int count = 0;
        for (const auto& result : m_results) {
            if (result.passed) count++;
        }
        return count;
    }

    int getFailedCount() const {
        int count = 0;
        for (const auto& result : m_results) {
            if (!result.passed) count++;
        }
        return count;
    }

    std::string m_suite_name;
    std::vector<Test> m_tests;
    std::vector<TestResult> m_results;
};

// 测试断言宏
#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "  ❌ FAILED: " << #condition << " at line " << __LINE__ << std::endl; \
        return false; \
    } else { \
        std::cout << "  ✓ PASSED: " << #condition << std::endl; \
    }

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))
#define ASSERT_GE(a, b) ASSERT_TRUE((a) >= (b))
#define ASSERT_LE(a, b) ASSERT_TRUE((a) <= (b))

// 测试文件路径
const char* TEST_FILE = "/tmp/zerocp_posix_test.txt";
const char* TEST_CONTENT = "Hello, ZeroCp Framework!";

// ==================== 基础功能测试 ====================

/**
 * 测试 1: open() 使用 failureReturnValue
 * 验证：打开文件成功，返回有效的文件描述符
 */
bool test_open_with_failure_value() {
    auto eval = ZeroCp_PosixCall(open)(TEST_FILE, O_CREAT | O_RDWR, 0644)
                   .failureReturnValue(-1);
    
    ASSERT_TRUE(eval.hasSuccess());
    
    int fd = eval.getValue();
    ASSERT_GE(fd, 0);
    ASSERT_EQ(eval.getErrnum(), 0);
    
    close(fd);
    unlink(TEST_FILE);
    return true;
}

/**
 * 测试 2: open() 失败情况
 * 验证：打开不存在的路径时正确返回失败
 */
bool test_open_failure() {
    auto eval = ZeroCp_PosixCall(open)("/nonexistent/impossible/path/file.txt", O_RDONLY)
                   .failureReturnValue(-1);
    
    ASSERT_FALSE(eval.hasSuccess());
    ASSERT_EQ(eval.getValue(), -1);
    ASSERT_NE(eval.getErrnum(), 0);
    
    std::cout << "  ℹ Error: errno=" << eval.getErrnum() 
              << " (" << strerror(eval.getErrnum()) << ")" << std::endl;
    
    return true;
}

/**
 * 测试 3: close() 使用 successReturnValue
 * 验证：关闭文件成功返回 0
 */
bool test_close_with_success_value() {
    // 先打开文件
    int fd = open(TEST_FILE, O_CREAT | O_RDWR, 0644);
    ASSERT_GE(fd, 0);
    
    // 使用框架关闭
    auto eval = ZeroCp_PosixCall(close)(fd)
                   .successReturnValue(0);
    
    ASSERT_TRUE(eval.hasSuccess());
    ASSERT_EQ(eval.getValue(), 0);
    
    unlink(TEST_FILE);
    return true;
}

/**
 * 测试 4: close() 失败情况
 * 验证：关闭无效 fd 时正确返回失败
 */
bool test_close_failure() {
    int invalid_fd = -1;
    auto eval = ZeroCp_PosixCall(close)(invalid_fd)
                   .successReturnValue(0);
    
    ASSERT_FALSE(eval.hasSuccess());
    ASSERT_EQ(eval.getValue(), -1);
    ASSERT_NE(eval.getErrnum(), 0);
    
    return true;
}

/**
 * 测试 5: write() 操作
 * 验证：写入数据成功，返回写入字节数
 */
bool test_write() {
    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ASSERT_GE(fd, 0);
    
    auto eval = ZeroCp_PosixCall(write)(fd, TEST_CONTENT, strlen(TEST_CONTENT))
                   .failureReturnValue(-1);
    
    ASSERT_TRUE(eval.hasSuccess());
    
    ssize_t bytes_written = eval.getValue();
    ASSERT_EQ(bytes_written, static_cast<ssize_t>(strlen(TEST_CONTENT)));
    
    std::cout << "  ℹ Wrote " << bytes_written << " bytes" << std::endl;
    
    close(fd);
    unlink(TEST_FILE);
    return true;
}

/**
 * 测试 6: read() 操作
 * 验证：读取数据成功，内容正确
 */
bool test_read() {
    // 先写入测试内容
    int fd_write = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd_write, TEST_CONTENT, strlen(TEST_CONTENT));
    close(fd_write);
    
    // 读取文件
    int fd = open(TEST_FILE, O_RDONLY);
    ASSERT_GE(fd, 0);
    
    char buffer[1024] = {0};
    auto eval = ZeroCp_PosixCall(read)(fd, buffer, sizeof(buffer))
                   .failureReturnValue(-1);
    
    ASSERT_TRUE(eval.hasSuccess());
    
    ssize_t bytes_read = eval.getValue();
    ASSERT_EQ(bytes_read, static_cast<ssize_t>(strlen(TEST_CONTENT)));
    ASSERT_EQ(std::string(buffer), std::string(TEST_CONTENT));
    
    std::cout << "  ℹ Read: \"" << buffer << "\"" << std::endl;
    
    close(fd);
    unlink(TEST_FILE);
    return true;
}

/**
 * 测试 7: lseek() 操作
 * 验证：文件定位成功
 */
bool test_lseek() {
    // 写入测试内容
    int fd_write = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd_write, TEST_CONTENT, strlen(TEST_CONTENT));
    close(fd_write);
    
    // 定位到文件末尾
    int fd = open(TEST_FILE, O_RDONLY);
    ASSERT_GE(fd, 0);
    
    auto eval = ZeroCp_PosixCall(lseek)(fd, 0, SEEK_END)
                   .failureReturnValue(-1);
    
    ASSERT_TRUE(eval.hasSuccess());
    
    off_t file_size = eval.getValue();
    ASSERT_EQ(file_size, static_cast<off_t>(strlen(TEST_CONTENT)));
    
    std::cout << "  ℹ File size: " << file_size << " bytes" << std::endl;
    
    close(fd);
    unlink(TEST_FILE);
    return true;
}

/**
 * 测试 8: unlink() 操作
 * 验证：删除文件成功
 */
bool test_unlink() {
    // 创建文件
    int fd = open(TEST_FILE, O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);
    close(fd);
    
    // 删除文件
    auto eval = ZeroCp_PosixCall(unlink)(TEST_FILE)
                   .successReturnValue(0);
    
    ASSERT_TRUE(eval.hasSuccess());
    ASSERT_EQ(eval.getValue(), 0);
    
    // 验证文件已被删除
    ASSERT_EQ(access(TEST_FILE, F_OK), -1);
    
    return true;
}

// ==================== 高级功能测试 ====================

/**
 * 测试 9: 可变参数 - 多个成功值
 * 验证：successReturnValue 可以接受多个值
 */
bool test_multiple_success_values() {
    // 创建测试文件
    int fd = open(TEST_FILE, O_CREAT | O_WRONLY, 0644);
    close(fd);
    
    // access() 返回 0 表示成功
    auto eval = ZeroCp_PosixCall(access)(TEST_FILE, F_OK)
                   .successReturnValue(0, 1);  // 允许 0 或 1
    
    ASSERT_TRUE(eval.hasSuccess());
    ASSERT_EQ(eval.getValue(), 0);
    
    unlink(TEST_FILE);
    return true;
}

/**
 * 测试 10: 可变参数 - 多个失败值
 * 验证：failureReturnValue 可以接受多个值
 */
bool test_multiple_failure_values() {
    auto eval = ZeroCp_PosixCall(open)(TEST_FILE, O_CREAT | O_RDWR, 0644)
                   .failureReturnValue(-1, -2, -3);  // 排除多个失败值
    
    ASSERT_TRUE(eval.hasSuccess());
    
    int fd = eval.getValue();
    ASSERT_GE(fd, 0);
    
    close(fd);
    unlink(TEST_FILE);
    return true;
}

/**
 * 测试 11: 完整工作流
 * 验证：链式调用完整流程正常工作
 */
bool test_complete_workflow() {
    // 1. 打开文件
    auto open_eval = ZeroCp_PosixCall(open)(TEST_FILE, O_CREAT | O_RDWR | O_TRUNC, 0644)
                        .failureReturnValue(-1);
    ASSERT_TRUE(open_eval.hasSuccess());
    int fd = open_eval.getValue();
    std::cout << "  ✓ Step 1: Opened file (fd=" << fd << ")" << std::endl;
    
    // 2. 写入数据
    const char* data = "ZeroCp Test Data 123456789";
    auto write_eval = ZeroCp_PosixCall(write)(fd, data, strlen(data))
                         .failureReturnValue(-1);
    ASSERT_TRUE(write_eval.hasSuccess());
    ASSERT_EQ(write_eval.getValue(), static_cast<ssize_t>(strlen(data)));
    std::cout << "  ✓ Step 2: Wrote " << write_eval.getValue() << " bytes" << std::endl;
    
    // 3. 重置文件指针
    auto lseek_eval = ZeroCp_PosixCall(lseek)(fd, 0, SEEK_SET)
                         .failureReturnValue(-1);
    ASSERT_TRUE(lseek_eval.hasSuccess());
    ASSERT_EQ(lseek_eval.getValue(), 0);
    std::cout << "  ✓ Step 3: Reset file pointer" << std::endl;
    
    // 4. 读取数据
    char buffer[1024] = {0};
    auto read_eval = ZeroCp_PosixCall(read)(fd, buffer, sizeof(buffer))
                        .failureReturnValue(-1);
    ASSERT_TRUE(read_eval.hasSuccess());
    ASSERT_EQ(std::string(buffer), std::string(data));
    std::cout << "  ✓ Step 4: Read and verified data" << std::endl;
    
    // 5. 获取文件大小
    auto lseek_end = ZeroCp_PosixCall(lseek)(fd, 0, SEEK_END)
                        .failureReturnValue(-1);
    ASSERT_TRUE(lseek_end.hasSuccess());
    ASSERT_EQ(lseek_end.getValue(), static_cast<off_t>(strlen(data)));
    std::cout << "  ✓ Step 5: File size = " << lseek_end.getValue() << " bytes" << std::endl;
    
    // 6. 关闭文件
    auto close_eval = ZeroCp_PosixCall(close)(fd)
                         .successReturnValue(0);
    ASSERT_TRUE(close_eval.hasSuccess());
    std::cout << "  ✓ Step 6: Closed file" << std::endl;
    
    // 7. 删除文件
    auto unlink_eval = ZeroCp_PosixCall(unlink)(TEST_FILE)
                          .successReturnValue(0);
    ASSERT_TRUE(unlink_eval.hasSuccess());
    std::cout << "  ✓ Step 7: Deleted file" << std::endl;
    
    return true;
}

/**
 * 测试 12: 边界条件 - 零字节写入
 * 验证：写入 0 字节的边界情况
 */
bool test_zero_byte_write() {
    int fd = open(TEST_FILE, O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);
    
    auto eval = ZeroCp_PosixCall(write)(fd, "", 0)
                   .failureReturnValue(-1);
    
    ASSERT_TRUE(eval.hasSuccess());
    ASSERT_EQ(eval.getValue(), 0);
    
    close(fd);
    unlink(TEST_FILE);
    return true;
}

/**
 * 测试 13: 类型推导
 * 验证：不同返回类型的正确推导
 */
bool test_type_deduction() {
    // open() 返回 int
    auto open_eval = ZeroCp_PosixCall(open)(TEST_FILE, O_CREAT | O_RDWR, 0644)
                        .failureReturnValue(-1);
    ASSERT_TRUE(open_eval.hasSuccess());
    int fd = open_eval.getValue();
    
    // write() 返回 ssize_t
    auto write_eval = ZeroCp_PosixCall(write)(fd, "test", 4)
                         .failureReturnValue(-1);
    ASSERT_TRUE(write_eval.hasSuccess());
    ssize_t bytes = write_eval.getValue();
    ASSERT_EQ(bytes, 4);
    
    // lseek() 返回 off_t
    auto lseek_eval = ZeroCp_PosixCall(lseek)(fd, 0, SEEK_END)
                         .failureReturnValue(-1);
    ASSERT_TRUE(lseek_eval.hasSuccess());
    off_t pos = lseek_eval.getValue();
    ASSERT_EQ(pos, 4);
    
    close(fd);
    unlink(TEST_FILE);
    return true;
}

// ==================== 主函数 ====================

int main() {
    TestRunner runner("ZeroCp POSIX Call Framework Tests");
    
    // 基础功能测试
    runner.addTest("Open with failure value", test_open_with_failure_value);
    runner.addTest("Open failure case", test_open_failure);
    runner.addTest("Close with success value", test_close_with_success_value);
    runner.addTest("Close failure case", test_close_failure);
    runner.addTest("Write operation", test_write);
    runner.addTest("Read operation", test_read);
    runner.addTest("Lseek operation", test_lseek);
    runner.addTest("Unlink operation", test_unlink);
    
    // 高级功能测试
    runner.addTest("Multiple success values", test_multiple_success_values);
    runner.addTest("Multiple failure values", test_multiple_failure_values);
    runner.addTest("Complete workflow", test_complete_workflow);
    runner.addTest("Zero byte write", test_zero_byte_write);
    runner.addTest("Type deduction", test_type_deduction);
    
    // 运行所有测试
    int result = runner.runAll();
    
    // 清理可能残留的临时文件
    unlink(TEST_FILE);
    
    return result;
}

