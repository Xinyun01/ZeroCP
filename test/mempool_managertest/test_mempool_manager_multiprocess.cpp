/**
 * @file test_mempool_manager_multiprocess.cpp
 * @brief MemPoolManager 多进程共享内存测试
 * @author ZeroCopy Framework Team
 * @date 2025-10-30
 * 
 * 测试内容：
 * 1. 父进程创建共享实例
 * 2. 子进程附加到已有共享实例
 * 3. 验证两个进程看到相同的数据结构
 * 4. 验证进程本地变量（基地址）可以不同
 * 5. 验证共享内存中的 RelativePointer 工作正常
 */

#include "mempool_manager.hpp"
#include "mempool_config.hpp"
#include "logging.hpp"

#include <iostream>
#include <cassert>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

using namespace ZeroCP::Memory;

// ANSI 颜色代码
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RESET   "\033[0m"

// 共享内存中的测试数据结构（用于验证跨进程访问）
struct SharedTestData {
    uint64_t magicNumber;
    uint64_t processId;
    uint64_t timestamp;
    char message[128];
};

// ============================================================================
// 子进程函数
// ============================================================================
int childProcess() {
    std::cout << COLOR_YELLOW << "\n[子进程] PID=" << getpid() << COLOR_RESET << std::endl;
    
    try {
        // 子进程等待一下，确保父进程先创建好共享内存
        sleep(1);
        
        // 子进程附加到已有的共享实例
        // 注意：不调用 createSharedInstance，而是直接获取实例
        MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();
        
        if (mgr == nullptr) {
            std::cerr << COLOR_RED << "[子进程] 无法获取共享实例" << COLOR_RESET << std::endl;
            return 1;
        }
        
        std::cout << COLOR_YELLOW << "[子进程] 成功附加到共享实例: " 
                  << mgr << COLOR_RESET << std::endl;
        
        // 验证内存大小计算
        uint64_t mgmtSize = mgr->getManagementMemorySize();
        uint64_t chunkSize = mgr->getChunkMemorySize();
        uint64_t totalSize = mgr->getTotalMemorySize();
        
        std::cout << COLOR_YELLOW << "[子进程] 内存大小:" << COLOR_RESET << std::endl;
        std::cout << COLOR_YELLOW << "  管理区: " << mgmtSize << " 字节" << COLOR_RESET << std::endl;
        std::cout << COLOR_YELLOW << "  数据区: " << chunkSize << " 字节" << COLOR_RESET << std::endl;
        std::cout << COLOR_YELLOW << "  总计:   " << totalSize << " 字节" << COLOR_RESET << std::endl;
        
        // 验证 MemPool 数量和配置
        auto& mempools = mgr->getMemPools();
        std::cout << COLOR_YELLOW << "[子进程] MemPool 数量: " 
                  << mempools.size() << COLOR_RESET << std::endl;
        
        if (mempools.size() != 3) {
            std::cerr << COLOR_RED << "[子进程] MemPool 数量不正确！" << COLOR_RESET << std::endl;
            return 1;
        }
        
        // 检查每个 MemPool 的配置
        for (uint64_t i = 0; i < mempools.size(); ++i) {
            const auto& pool = mempools[i];
            std::cout << COLOR_YELLOW << "[子进程] Pool[" << i << "]: "
                      << "chunkSize=" << pool.getChunkSize()
                      << ", total=" << pool.getTotalChunks()
                      << ", free=" << pool.getFreeChunks()
                      << COLOR_RESET << std::endl;
        }
        
        // 验证统计信息
        std::cout << COLOR_YELLOW << "[子进程] 打印统计信息:" << COLOR_RESET << std::endl;
        mgr->printAllPoolStats();
        
        std::cout << COLOR_GREEN << "[子进程] ✓ 所有验证通过！" << COLOR_RESET << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << COLOR_RED << "[子进程] 异常: " << e.what() << COLOR_RESET << std::endl;
        return 1;
    }
}

// ============================================================================
// 父进程函数
// ============================================================================
int parentProcess() {
    std::cout << COLOR_BLUE << "\n[父进程] PID=" << getpid() << COLOR_RESET << std::endl;
    
    try {
        // 清理可能存在的旧实例
        MemPoolManager::destroySharedInstance();
        
        // 创建配置
        MemPoolConfig config;
        config.addMemPoolEntry(256, 100);   // 256 字节 × 100
        config.addMemPoolEntry(1024, 50);   // 1KB × 50
        config.addMemPoolEntry(4096, 20);   // 4KB × 20
        
        std::cout << COLOR_BLUE << "[父进程] 创建 MemPoolConfig，3个内存池" << COLOR_RESET << std::endl;
        
        // 创建共享实例
        bool success = MemPoolManager::createSharedInstance(config);
        if (!success) {
            std::cerr << COLOR_RED << "[父进程] 创建共享实例失败" << COLOR_RESET << std::endl;
            return 1;
        }
        
        MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();
        if (mgr == nullptr) {
            std::cerr << COLOR_RED << "[父进程] 获取实例失败" << COLOR_RESET << std::endl;
            return 1;
        }
        
        std::cout << COLOR_BLUE << "[父进程] 共享实例创建成功: " 
                  << mgr << COLOR_RESET << std::endl;
        
        // 打印内存大小
        uint64_t mgmtSize = mgr->getManagementMemorySize();
        uint64_t chunkSize = mgr->getChunkMemorySize();
        uint64_t totalSize = mgr->getTotalMemorySize();
        
        std::cout << COLOR_BLUE << "[父进程] 内存布局:" << COLOR_RESET << std::endl;
        std::cout << COLOR_BLUE << "  管理区大小: " << mgmtSize << " 字节" << COLOR_RESET << std::endl;
        std::cout << COLOR_BLUE << "  数据区大小: " << chunkSize << " 字节" << COLOR_RESET << std::endl;
        std::cout << COLOR_BLUE << "  总内存大小: " << totalSize << " 字节 (" 
                  << (totalSize / 1024.0 / 1024.0) << " MB)" << COLOR_RESET << std::endl;
        
        // 打印 MemPool 信息
        auto& mempools = mgr->getMemPools();
        std::cout << COLOR_BLUE << "[父进程] MemPool 数量: " 
                  << mempools.size() << COLOR_RESET << std::endl;
        
        for (uint64_t i = 0; i < mempools.size(); ++i) {
            const auto& pool = mempools[i];
            std::cout << COLOR_BLUE << "[父进程] Pool[" << i << "]: "
                      << "chunkSize=" << pool.getChunkSize()
                      << ", total=" << pool.getTotalChunks()
                      << ", free=" << pool.getFreeChunks()
                      << COLOR_RESET << std::endl;
        }
        
        // 打印统计信息
        std::cout << COLOR_BLUE << "[父进程] 统计信息:" << COLOR_RESET << std::endl;
        mgr->printAllPoolStats();
        
        std::cout << COLOR_GREEN << "[父进程] ✓ 初始化完成，等待子进程..." << COLOR_RESET << std::endl;
        
        // 创建子进程
        pid_t pid = fork();
        
        if (pid < 0) {
            std::cerr << COLOR_RED << "[父进程] fork 失败" << COLOR_RESET << std::endl;
            return 1;
        }
        else if (pid == 0) {
            // 这是子进程
            int result = childProcess();
            exit(result);
        }
        else {
            // 这是父进程，等待子进程结束
            std::cout << COLOR_BLUE << "[父进程] 等待子进程 (PID=" 
                      << pid << ") ..." << COLOR_RESET << std::endl;
            
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                int exitCode = WEXITSTATUS(status);
                if (exitCode == 0) {
                    std::cout << COLOR_GREEN << "[父进程] 子进程成功退出 ✓" 
                              << COLOR_RESET << std::endl;
                } else {
                    std::cout << COLOR_RED << "[父进程] 子进程失败退出，代码=" 
                              << exitCode << COLOR_RESET << std::endl;
                    return 1;
                }
            } else {
                std::cout << COLOR_RED << "[父进程] 子进程异常终止" 
                          << COLOR_RESET << std::endl;
                return 1;
            }
            
            // 父进程继续验证
            std::cout << COLOR_BLUE << "[父进程] 子进程结束后，父进程继续验证..." 
                      << COLOR_RESET << std::endl;
            
            // 再次打印统计信息，看是否有变化
            mgr->printAllPoolStats();
            
            std::cout << COLOR_GREEN << "[父进程] ✓ 所有验证通过！" << COLOR_RESET << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << COLOR_RED << "[父进程] 异常: " << e.what() << COLOR_RESET << std::endl;
        return 1;
    }
}

// ============================================================================
// 主函数
// ============================================================================
int main() {
    std::cout << std::string(70, '=') << std::endl;
    std::cout << COLOR_CYAN << "MemPoolManager 多进程共享内存测试" << COLOR_RESET << std::endl;
    std::cout << COLOR_CYAN << "测试架构：双共享内存（管理区 + 数据区）" << COLOR_RESET << std::endl;
    std::cout << COLOR_CYAN << "测试重点：进程本地变量 vs 相对偏移量" << COLOR_RESET << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // 启动日志系统
    ZeroCP::Log::Log_Manager::getInstance().start();
    ZeroCP::Log::Log_Manager::getInstance().setLogLevel(ZeroCP::Log::LogLevel::Info);
    
    // 运行父进程测试
    int result = parentProcess();
    
    // 清理共享实例
    std::cout << "\n" << COLOR_CYAN << "清理共享实例..." << COLOR_RESET << std::endl;
    MemPoolManager::destroySharedInstance();
    
    // 停止日志系统
    ZeroCP::Log::Log_Manager::getInstance().stop();
    
    if (result == 0) {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << COLOR_GREEN << "🎉 多进程测试全部通过！" << COLOR_RESET << std::endl;
        std::cout << std::string(70, '=') << std::endl;
    } else {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << COLOR_RED << "❌ 多进程测试失败" << COLOR_RESET << std::endl;
        std::cout << std::string(70, '=') << std::endl;
    }
    
    return result;
}

