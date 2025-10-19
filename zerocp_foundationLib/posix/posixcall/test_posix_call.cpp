// 测试 PosixCall 模块
#include "include/posix_call.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

using namespace ZeroCp;

// C++20: 包装可变参数函数为固定参数版本
// open() 是一个可变参数函数，我们需要包装它
inline int open_wrapper(const char* pathname, int flags) noexcept
{
    return ::open(pathname, flags);
}

inline int open_wrapper3(const char* pathname, int flags, mode_t mode) noexcept
{
    return ::open(pathname, flags, mode);
}

int main()
{
    std::cout << "=== Testing PosixCall Module (C++20/23 Edition) ===" << std::endl;
    
    // 测试 1: 成功的系统调用 (打开 /dev/null)
    std::cout << "\n[Test 1] Opening /dev/null (should succeed)..." << std::endl;
    auto result1 = ZeroCp_PosixCall(open_wrapper)("/dev/null", O_RDONLY)
        .failureReturnValue(-1)
        .evaluate();
    
    if (result1.has_value())
    {
        std::cout << "✓ Success: fd = " << result1.value().value << std::endl;
        close(result1.value().value);
    }
    else
    {
        std::cout << "✗ Failed: errno = " << result1.error().errnum 
                  << " (" << strerror(result1.error().errnum) << ")" << std::endl;
    }
    
    // 测试 2: 失败的系统调用 (打开不存在的文件)
    std::cout << "\n[Test 2] Opening non-existent file (should fail)..." << std::endl;
    auto result2 = ZeroCp_PosixCall(open_wrapper)("/this/path/does/not/exist", O_RDONLY)
        .failureReturnValue(-1)
        .evaluate();
    
    if (result2.has_value())
    {
        std::cout << "✗ Unexpected success: fd = " << result2.value().value << std::endl;
        close(result2.value().value);
    }
    else
    {
        std::cout << "✓ Expected failure: errno = " << result2.error().errnum 
                  << " (" << strerror(result2.error().errnum) << ")" << std::endl;
    }
    
    // 测试 3: 使用 suppressErrorMessagesForErrnos 忽略特定错误
    std::cout << "\n[Test 3] Opening with suppressed error messages..." << std::endl;
    auto result3 = ZeroCp_PosixCall(open_wrapper)("/another/non/existent/path", O_RDONLY)
        .failureReturnValue(-1)
        .suppressErrorMessagesForErrnos(ENOENT)
        .evaluate();
    
    if (result3.has_value())
    {
        std::cout << "✗ Unexpected success" << std::endl;
        close(result3.value().value);
    }
    else
    {
        std::cout << "✓ Expected failure (error message suppressed): errno = " 
                  << result3.error().errnum << std::endl;
    }
    
    // 测试 4: 测试 getpid (总是成功)
    std::cout << "\n[Test 4] Getting process ID..." << std::endl;
    auto result4 = ZeroCp_PosixCall(getpid)()
        .returnValueMatchesErrno()
        .evaluate();
    
    if (result4.has_value())
    {
        std::cout << "✓ Success: PID = " << result4.value().value << std::endl;
    }
    else
    {
        std::cout << "✗ Failed (unexpected)" << std::endl;
    }
    
    // 测试 5: 测试 write 系统调用
    std::cout << "\n[Test 5] Writing to stdout..." << std::endl;
    const char* message = "Hello from PosixCall!\n";
    auto result5 = ZeroCp_PosixCall(write)(STDOUT_FILENO, message, strlen(message))
        .failureReturnValue(-1)
        .evaluate();
    
    if (result5.has_value())
    {
        std::cout << "✓ Success: wrote " << result5.value().value << " bytes" << std::endl;
    }
    else
    {
        std::cout << "✗ Failed: errno = " << result5.error().errnum << std::endl;
    }
    
    std::cout << "\n=== All tests completed ===" << std::endl;
    return 0;
}

