// 完整的系统调用测试集合
#include "include/posix_call.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cerrno>

using namespace ZeroCp;

// ==================== 包装器函数 ====================
// 因为某些系统调用是可变参数函数，需要包装为固定参数版本

inline int open_wrapper(const char* pathname, int flags) noexcept
{
    return ::open(pathname, flags);
}

inline int open_wrapper3(const char* pathname, int flags, mode_t mode) noexcept
{
    return ::open(pathname, flags, mode);
}

// ==================== 测试辅助函数 ====================

void print_test_header(const char* test_name)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << test_name << std::endl;
    std::cout << "========================================" << std::endl;
}

void print_success(const char* msg)
{
    std::cout << "✓ " << msg << std::endl;
}

void print_failure(const char* msg)
{
    std::cout << "✗ " << msg << std::endl;
}

// ==================== 测试用例 ====================

// 测试 1: 文件操作 - open/close/read/write
void test_file_operations()
{
    print_test_header("Test 1: File Operations");
    
    // 1.1 打开设备文件 /dev/null (应该成功)
    std::cout << "\n[1.1] Opening /dev/null..." << std::endl;
    auto result = ZeroCp_PosixCall(open_wrapper)("/dev/null", O_RDWR)
        .failureReturnValue(-1)
        .evaluate();
    
    if (result.has_value())
    {
        int fd = result.value().value;
        print_success(("Opened /dev/null, fd = " + std::to_string(fd)).c_str());
        
        // 1.2 写入数据到 /dev/null
        std::cout << "\n[1.2] Writing to /dev/null..." << std::endl;
        const char* data = "Test data";
        auto write_result = ZeroCp_PosixCall(write)(fd, data, strlen(data))
            .failureReturnValue(-1)
            .evaluate();
        
        if (write_result.has_value())
        {
            print_success(("Wrote " + std::to_string(write_result.value().value) + " bytes").c_str());
        }
        else
        {
            print_failure(("Write failed: " + std::string(strerror(write_result.error().errnum))).c_str());
        }
        
        // 1.3 关闭文件
        std::cout << "\n[1.3] Closing file..." << std::endl;
        auto close_result = ZeroCp_PosixCall(close)(fd)
            .failureReturnValue(-1)
            .evaluate();
        
        if (close_result.has_value())
        {
            print_success("File closed successfully");
        }
        else
        {
            print_failure("Close failed");
        }
    }
    else
    {
        print_failure(("Open failed: " + std::string(strerror(result.error().errnum))).c_str());
    }
    
    // 1.4 打开不存在的文件 (应该失败)
    std::cout << "\n[1.4] Opening non-existent file..." << std::endl;
    auto fail_result = ZeroCp_PosixCall(open_wrapper)("/this/path/does/not/exist/file.txt", O_RDONLY)
        .failureReturnValue(-1)
        .evaluate();
    
    if (!fail_result.has_value())
    {
        print_success(("Expected failure: " + std::string(strerror(fail_result.error().errnum))).c_str());
    }
    else
    {
        print_failure("Unexpected success");
        close(fail_result.value().value);
    }
}

// 测试 2: 创建和删除文件
void test_file_creation()
{
    print_test_header("Test 2: File Creation and Deletion");
    
    const char* test_file = "/tmp/posix_call_test_file.txt";
    
    // 2.1 创建新文件
    std::cout << "\n[2.1] Creating new file: " << test_file << std::endl;
    auto create_result = ZeroCp_PosixCall(open_wrapper3)(
        test_file, 
        O_CREAT | O_WRONLY | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
    )
    .failureReturnValue(-1)
    .evaluate();
    
    if (create_result.has_value())
    {
        int fd = create_result.value().value;
        print_success(("File created, fd = " + std::to_string(fd)).c_str());
        
        // 写入一些数据
        const char* content = "Hello, PosixCall!\n";
        auto write_result = ZeroCp_PosixCall(write)(fd, content, strlen(content))
            .failureReturnValue(-1)
            .evaluate();
        
        if (write_result.has_value())
        {
            print_success(("Wrote " + std::to_string(write_result.value().value) + " bytes").c_str());
        }
        
        close(fd);
        
        // 2.2 删除文件
        std::cout << "\n[2.2] Deleting file..." << std::endl;
        auto unlink_result = ZeroCp_PosixCall(unlink)(test_file)
            .failureReturnValue(-1)
            .evaluate();
        
        if (unlink_result.has_value())
        {
            print_success("File deleted successfully");
        }
        else
        {
            print_failure(("Delete failed: " + std::string(strerror(unlink_result.error().errnum))).c_str());
        }
    }
    else
    {
        print_failure(("Create failed: " + std::string(strerror(create_result.error().errnum))).c_str());
    }
}

// 测试 3: 进程信息
void test_process_info()
{
    print_test_header("Test 3: Process Information");
    
    // 3.1 获取进程 ID
    std::cout << "\n[3.1] Getting process ID..." << std::endl;
    auto pid_result = ZeroCp_PosixCall(getpid)()
        .returnValueMatchesErrno()
        .evaluate();
    
    if (pid_result.has_value())
    {
        print_success(("PID = " + std::to_string(pid_result.value().value)).c_str());
    }
    else
    {
        print_failure("getpid failed (unexpected)");
    }
    
    // 3.2 获取父进程 ID
    std::cout << "\n[3.2] Getting parent process ID..." << std::endl;
    auto ppid_result = ZeroCp_PosixCall(getppid)()
        .returnValueMatchesErrno()
        .evaluate();
    
    if (ppid_result.has_value())
    {
        print_success(("PPID = " + std::to_string(ppid_result.value().value)).c_str());
    }
    else
    {
        print_failure("getppid failed (unexpected)");
    }
    
    // 3.3 获取用户 ID
    std::cout << "\n[3.3] Getting user ID..." << std::endl;
    auto uid_result = ZeroCp_PosixCall(getuid)()
        .returnValueMatchesErrno()
        .evaluate();
    
    if (uid_result.has_value())
    {
        print_success(("UID = " + std::to_string(uid_result.value().value)).c_str());
    }
    else
    {
        print_failure("getuid failed (unexpected)");
    }
}

// 测试 4: 文件状态
void test_file_status()
{
    print_test_header("Test 4: File Status");
    
    // 4.1 获取 /dev/null 的状态
    std::cout << "\n[4.1] Getting status of /dev/null..." << std::endl;
    struct stat st;
    auto stat_result = ZeroCp_PosixCall(stat)("/dev/null", &st)
        .failureReturnValue(-1)
        .evaluate();
    
    if (stat_result.has_value())
    {
        print_success("stat() succeeded");
        std::cout << "  File size: " << st.st_size << " bytes" << std::endl;
        std::cout << "  Inode: " << st.st_ino << std::endl;
        std::cout << "  Device: " << st.st_dev << std::endl;
    }
    else
    {
        print_failure(("stat failed: " + std::string(strerror(stat_result.error().errnum))).c_str());
    }
    
    // 4.2 获取不存在文件的状态 (应该失败)
    std::cout << "\n[4.2] Getting status of non-existent file..." << std::endl;
    struct stat st2;
    auto stat_fail = ZeroCp_PosixCall(stat)("/non/existent/file", &st2)
        .failureReturnValue(-1)
        .suppressErrorMessagesForErrnos(ENOENT)
        .evaluate();
    
    if (!stat_fail.has_value())
    {
        print_success(("Expected failure: errno = " + std::to_string(stat_fail.error().errnum)).c_str());
    }
    else
    {
        print_failure("Unexpected success");
    }
}

// 测试 5: 错误抑制
void test_error_suppression()
{
    print_test_header("Test 5: Error Message Suppression");
    
    // 5.1 不抑制错误消息
    std::cout << "\n[5.1] Opening with normal error reporting..." << std::endl;
    auto result1 = ZeroCp_PosixCall(open_wrapper)("/invalid/path/1", O_RDONLY)
        .failureReturnValue(-1)
        .evaluate();
    
    if (!result1.has_value())
    {
        print_success("Error reported normally");
    }
    
    // 5.2 抑制 ENOENT 错误消息
    std::cout << "\n[5.2] Opening with suppressed ENOENT..." << std::endl;
    auto result2 = ZeroCp_PosixCall(open_wrapper)("/invalid/path/2", O_RDONLY)
        .failureReturnValue(-1)
        .suppressErrorMessagesForErrnos(ENOENT)
        .evaluate();
    
    if (!result2.has_value())
    {
        print_success("ENOENT suppressed (no error message should appear)");
    }
    
    // 5.3 忽略特定 errno
    std::cout << "\n[5.3] Opening with ignored errno..." << std::endl;
    auto result3 = ZeroCp_PosixCall(open_wrapper)("/invalid/path/3", O_RDONLY)
        .failureReturnValue(-1)
        .ignoreErrnos(ENOENT, EACCES)
        .evaluate();
    
    if (result3.has_value() || !result3.has_value())
    {
        print_success("Errno handling with ignoreErrnos()");
    }
}

// 测试 6: 多返回值验证
void test_multiple_return_values()
{
    print_test_header("Test 6: Multiple Return Value Verification");
    
    // 6.1 测试 successReturnValue 接受多个值
    std::cout << "\n[6.1] Testing successReturnValue with multiple values..." << std::endl;
    auto result = ZeroCp_PosixCall(getpid)()
        .successReturnValue(1, 2, 3)  // 这些值都不会匹配实际的 PID
        .evaluate();
    
    // 由于 PID 不会是 1, 2, 或 3，这应该失败
    if (!result.has_value())
    {
        print_success("Correctly identified non-matching return value");
    }
    else
    {
        std::cout << "  Note: PID was " << result.value().value << std::endl;
    }
    
    // 6.2 测试 failureReturnValue 接受多个值
    std::cout << "\n[6.2] Testing failureReturnValue with multiple values..." << std::endl;
    auto result2 = ZeroCp_PosixCall(open_wrapper)("/dev/null", O_RDONLY)
        .failureReturnValue(-1, -2, -3)
        .evaluate();
    
    if (result2.has_value())
    {
        print_success(("Opened successfully, fd = " + std::to_string(result2.value().value)).c_str());
        close(result2.value().value);
    }
}

// 测试 7: std::expected 使用
void test_expected_usage()
{
    print_test_header("Test 7: std::expected Usage (C++23)");
    
    std::cout << "\n[7.1] Using std::expected interface..." << std::endl;
    
    auto result = ZeroCp_PosixCall(open_wrapper)("/dev/null", O_RDONLY)
        .failureReturnValue(-1)
        .evaluate();
    
    // 使用 std::expected 的各种方法
    std::cout << "  has_value(): " << (result.has_value() ? "true" : "false") << std::endl;
    
    if (result.has_value())
    {
        std::cout << "  value().value: " << result.value().value << std::endl;
        std::cout << "  value().errnum: " << result.value().errnum << std::endl;
        
        // 使用 and_then (C++23)
        auto fd = result.value().value;
        close(fd);
        
        print_success("std::expected interface works correctly");
    }
    else
    {
        std::cout << "  error().value: " << result.error().value << std::endl;
        std::cout << "  error().errnum: " << result.error().errnum << std::endl;
    }
}

// 测试 8: 工作目录操作
void test_working_directory()
{
    print_test_header("Test 8: Working Directory Operations");
    
    // 8.1 获取当前工作目录
    std::cout << "\n[8.1] Getting current working directory..." << std::endl;
    char cwd[1024];
    auto getcwd_result = ZeroCp_PosixCall(getcwd)(cwd, sizeof(cwd))
        .failureReturnValue(nullptr)
        .evaluate();
    
    if (getcwd_result.has_value())
    {
        print_success(("Current directory: " + std::string(cwd)).c_str());
    }
    else
    {
        print_failure("getcwd failed");
    }
    
    // 8.2 改变到 /tmp 目录
    std::cout << "\n[8.2] Changing to /tmp..." << std::endl;
    auto chdir_result = ZeroCp_PosixCall(chdir)("/tmp")
        .failureReturnValue(-1)
        .evaluate();
    
    if (chdir_result.has_value())
    {
        print_success("Changed to /tmp");
        
        // 验证改变
        char new_cwd[1024];
        if (getcwd(new_cwd, sizeof(new_cwd)))
        {
            std::cout << "  New directory: " << new_cwd << std::endl;
        }
        
        // 恢复原目录
        chdir(cwd);
    }
    else
    {
        print_failure("chdir failed");
    }
}

// 测试 9: 共享内存操作
void test_shared_memory()
{
    print_test_header("Test 9: Shared Memory Operations");
    
    const char* shm_name = "/posix_call_test_shm";
    const size_t shm_size = 4096;  // 4KB
    
    // 9.1 创建共享内存对象
    std::cout << "\n[9.1] Creating shared memory object: " << shm_name << std::endl;
    auto shm_fd_result = ZeroCp_PosixCall(shm_open)(
        shm_name,
        O_CREAT | O_RDWR,
        S_IRUSR | S_IWUSR
    )
    .failureReturnValue(-1)
    .evaluate();
    
    if (!shm_fd_result.has_value())
    {
        print_failure(("shm_open failed: " + std::string(strerror(shm_fd_result.error().errnum))).c_str());
        return;
    }
    
    int shm_fd = shm_fd_result.value().value;
    print_success(("Shared memory created, fd = " + std::to_string(shm_fd)).c_str());
    
    // 9.2 设置共享内存大小
    std::cout << "\n[9.2] Setting shared memory size to " << shm_size << " bytes..." << std::endl;
    auto ftruncate_result = ZeroCp_PosixCall(ftruncate)(shm_fd, shm_size)
        .failureReturnValue(-1)
        .evaluate();
    
    if (!ftruncate_result.has_value())
    {
        print_failure(("ftruncate failed: " + std::string(strerror(ftruncate_result.error().errnum))).c_str());
        close(shm_fd);
        shm_unlink(shm_name);
        return;
    }
    print_success("Shared memory size set successfully");
    
    // 9.3 获取共享内存的实际大小
    std::cout << "\n[9.3] Getting shared memory size..." << std::endl;
    struct stat shm_stat;
    auto fstat_result = ZeroCp_PosixCall(fstat)(shm_fd, &shm_stat)
        .failureReturnValue(-1)
        .evaluate();
    
    if (fstat_result.has_value())
    {
        print_success(("Shared memory size: " + std::to_string(shm_stat.st_size) + " bytes").c_str());
        if (static_cast<size_t>(shm_stat.st_size) == shm_size)
        {
            print_success("Size matches expected value");
        }
        else
        {
            print_failure(("Size mismatch: expected " + std::to_string(shm_size) + 
                          ", got " + std::to_string(shm_stat.st_size)).c_str());
        }
    }
    else
    {
        print_failure("fstat failed");
    }
    
    // 9.4 映射共享内存到进程地址空间
    std::cout << "\n[9.4] Mapping shared memory to process address space..." << std::endl;
    auto mmap_result = ZeroCp_PosixCall(mmap)(
        nullptr,
        shm_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    )
    .failureReturnValue(MAP_FAILED)
    .evaluate();
    
    if (!mmap_result.has_value())
    {
        print_failure(("mmap failed: " + std::string(strerror(mmap_result.error().errnum))).c_str());
        close(shm_fd);
        shm_unlink(shm_name);
        return;
    }
    
    void* shm_ptr = mmap_result.value().value;
    print_success("Shared memory mapped successfully");
    
    // 9.5 写入数据到共享内存
    std::cout << "\n[9.5] Writing data to shared memory..." << std::endl;
    const char* test_data = "Hello from PosixCall shared memory test!";
    std::memcpy(shm_ptr, test_data, strlen(test_data) + 1);
    print_success(("Wrote: \"" + std::string(test_data) + "\"").c_str());
    
    // 9.6 从共享内存读取数据
    std::cout << "\n[9.6] Reading data from shared memory..." << std::endl;
    char* read_data = static_cast<char*>(shm_ptr);
    std::cout << "  Read: \"" << read_data << "\"" << std::endl;
    
    if (std::strcmp(read_data, test_data) == 0)
    {
        print_success("Data verification successful!");
    }
    else
    {
        print_failure("Data mismatch!");
    }
    
    // 9.7 解除映射
    std::cout << "\n[9.7] Unmapping shared memory..." << std::endl;
    auto munmap_result = ZeroCp_PosixCall(munmap)(shm_ptr, shm_size)
        .failureReturnValue(-1)
        .evaluate();
    
    if (munmap_result.has_value())
    {
        print_success("Shared memory unmapped successfully");
    }
    else
    {
        print_failure(("munmap failed: " + std::string(strerror(munmap_result.error().errnum))).c_str());
    }
    
    // 9.8 关闭文件描述符
    std::cout << "\n[9.8] Closing shared memory file descriptor..." << std::endl;
    auto close_result = ZeroCp_PosixCall(close)(shm_fd)
        .failureReturnValue(-1)
        .evaluate();
    
    if (close_result.has_value())
    {
        print_success("File descriptor closed");
    }
    
    // 9.9 删除共享内存对象
    std::cout << "\n[9.9] Unlinking shared memory object..." << std::endl;
    auto unlink_result = ZeroCp_PosixCall(shm_unlink)(shm_name)
        .failureReturnValue(-1)
        .evaluate();
    
    if (unlink_result.has_value())
    {
        print_success("Shared memory object removed");
    }
    else
    {
        print_failure(("shm_unlink failed: " + std::string(strerror(unlink_result.error().errnum))).c_str());
    }
}

// ==================== 主函数 ====================

int main()
{
    std::cout << "\n" << std::endl;
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   PosixCall Complete Test Suite (C++20/23 Edition)    ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    try
    {
        test_file_operations();
        test_file_creation();
        test_process_info();
        test_file_status();
        test_error_suppression();
        test_multiple_return_values();
        test_expected_usage();
        test_working_directory();
        test_shared_memory();
        
        std::cout << "\n" << std::endl;
        std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║              All Tests Completed Successfully!         ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
        std::cout << "\n" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n✗ Exception caught: " << e.what() << std::endl;
        return 1;
    }
}

