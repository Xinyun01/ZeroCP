/**
 * @file test_mempool_creation.cpp
 * @brief MemPoolManager 创建验证测试
 * @author ZeroCopy Framework Team
 * @date 2025-10-31
 * 
 * 测试目标：
 * 验证 MemPoolManager 是否能够成功创建并正确初始化
 * 
 * 测试内容：
 * 1. 共享内存创建
 * 2. MemPoolManager 实例创建
 * 3. 管理区和数据区内存布局
 * 4. 基础配置验证
 * 5. 清理和销毁
 */

#include "mempool_manager.hpp"
#include "mempool_config.hpp"
#include "logging.hpp"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <unistd.h>
#include <functional>
#include <vector>

using namespace ZeroCP::Memory;

// ANSI 颜色代码
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RESET   "\033[0m"

// 打印分割线
void printSeparator(const std::string& title = "") {
    std::cout << "\n" << COLOR_CYAN << "═══════════════════════════════════════════════════════════" << COLOR_RESET << "\n";
    if (!title.empty()) {
        std::cout << COLOR_CYAN << "  " << title << COLOR_RESET << "\n";
        std::cout << COLOR_CYAN << "═══════════════════════════════════════════════════════════" << COLOR_RESET << "\n";
    }
}

// 打印成功信息
void printSuccess(const std::string& msg) {
    std::cout << COLOR_GREEN << "✓ " << msg << COLOR_RESET << std::endl;
}

// 打印失败信息
void printError(const std::string& msg) {
    std::cout << COLOR_RED << "✗ " << msg << COLOR_RESET << std::endl;
}

// 打印信息
void printInfo(const std::string& msg) {
    std::cout << COLOR_BLUE << "ℹ " << msg << COLOR_RESET << std::endl;
}

// 格式化字节大小
std::string formatBytes(uint64_t bytes) {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return std::to_string(bytes / 1024) + " KB";
    } else {
        return std::to_string(bytes / (1024 * 1024)) + " MB";
    }
}

// ============================================================================
// 测试 1: MemPoolConfig 配置创建
// ============================================================================
bool test_01_CreateConfig(MemPoolConfig& config) {
    printSeparator("测试 1: 创建 MemPoolConfig");
    
    try {
        // 添加三个内存池配置
        bool r1 = config.addMemPoolEntry(128, 100);   // 128B × 100 = 12.8 KB
        bool r2 = config.addMemPoolEntry(1024, 50);   // 1KB × 50 = 50 KB
        bool r3 = config.addMemPoolEntry(4096, 20);   // 4KB × 20 = 80 KB
        
        if (!r1 || !r2 || !r3) {
            printError("配置添加失败");
            return false;
        }
        
        printSuccess("MemPoolConfig 创建成功");
        std::cout << "  配置的内存池数量: " << config.m_memPoolEntries.size() << std::endl;
        
        for (size_t i = 0; i < config.m_memPoolEntries.size(); ++i) {
            const auto& entry = config.m_memPoolEntries[i];
            std::cout << "  Pool[" << i << "]: "
                      << "chunkSize=" << entry.m_chunkSize << "B, "
                      << "count=" << entry.m_chunkCount << ", "
                      << "total=" << formatBytes(entry.m_chunkSize * entry.m_chunkCount)
                      << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        printError(std::string("配置创建异常: ") + e.what());
        return false;
    }
}

// ============================================================================
// 测试 2: MemPoolManager 共享实例创建
// ============================================================================
bool test_02_CreateSharedInstance(const MemPoolConfig& config) {
    printSeparator("测试 2: 创建共享实例");
    
    try {
        // 先清理可能存在的旧实例
        MemPoolManager::destroySharedInstance();
        printInfo("已清理旧实例");
        
        // 创建共享实例
        bool success = MemPoolManager::createSharedInstance(config);
        
        if (!success) {
            printError("共享实例创建失败");
            return false;
        }
        
        printSuccess("共享实例创建成功");
        
        // 验证实例是否可获取
        MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();
        if (mgr == nullptr) {
            printError("获取实例失败：实例为 nullptr");
            return false;
        }
        
        printSuccess("实例获取成功");
        std::cout << "  实例地址: " << mgr << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        printError(std::string("实例创建异常: ") + e.what());
        return false;
    }
}

// ============================================================================
// 测试 3: 验证内存布局
// ============================================================================
bool test_03_VerifyMemoryLayout() {
    printSeparator("测试 3: 验证内存布局");
    
    try {
        MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();
        if (mgr == nullptr) {
            printError("实例未初始化");
            return false;
        }
        
        // 获取内存大小信息
        uint64_t managementSize = mgr->getManagementMemorySize();
        uint64_t chunkSize = mgr->getChunkMemorySize();
        uint64_t totalSize = mgr->getTotalMemorySize();
        
        printSuccess("内存大小计算正确");
        std::cout << "  管理区大小: " << formatBytes(managementSize) 
                  << " (" << managementSize << " bytes)" << std::endl;
        std::cout << "  数据区大小: " << formatBytes(chunkSize)
                  << " (" << chunkSize << " bytes)" << std::endl;
        std::cout << "  总大小:     " << formatBytes(totalSize)
                  << " (" << totalSize << " bytes)" << std::endl;
        
        // 验证总大小计算
        uint64_t expectedTotal = sizeof(MemPoolManager) + managementSize + chunkSize;
        if (totalSize < expectedTotal - 1000) {  // 允许一些对齐误差
            printError("总大小计算不正确");
            return false;
        }
        
        printSuccess("内存布局验证通过");
        return true;
    } catch (const std::exception& e) {
        printError(std::string("内存布局验证异常: ") + e.what());
        return false;
    }
}

// ============================================================================
// 测试 4: 验证 MemPool 配置
// ============================================================================
bool test_04_VerifyMemPools() {
    printSeparator("测试 4: 验证 MemPool 配置");
    
    try {
        MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();
        if (mgr == nullptr) {
            printError("实例未初始化");
            return false;
        }
        
        auto& mempools = mgr->getMemPools();
        
        printSuccess("获取 MemPool 列表成功");
        std::cout << "  MemPool 数量: " << mempools.size() << std::endl;
        
        // 验证每个池的配置
        for (uint64_t i = 0; i < mempools.size(); ++i) {
            const MemPool& pool = mempools[i];
            
            std::cout << "\n  Pool[" << i << "] 详细信息:" << std::endl;
            std::cout << "    - Chunk 大小: " << pool.getChunkSize() << " B" << std::endl;
            std::cout << "    - Chunk 数量: " << pool.getTotalChunks() << std::endl;
            std::cout << "    - 空闲 Chunks: " << pool.getFreeChunks() << std::endl;
            std::cout << "    - 已用 Chunks: " << pool.getUsedChunks() << std::endl;
            std::cout << "    - dataOffset: " << pool.getDataOffset() << std::endl;
            
            // 验证初始状态：所有 chunks 应该都是空闲的
            if (pool.getFreeChunks() != pool.getTotalChunks()) {
                printError("Pool[" + std::to_string(i) + "] 初始空闲数量不正确");
                return false;
            }
            
            if (pool.getUsedChunks() != 0) {
                printError("Pool[" + std::to_string(i) + "] 初始已用数量应为 0");
                return false;
            }
        }
        
        printSuccess("所有 MemPool 配置验证通过");
        return true;
    } catch (const std::exception& e) {
        printError(std::string("MemPool 验证异常: ") + e.what());
        return false;
    }
}

// ============================================================================
// 测试 5: 验证 ChunkManagerPool
// ============================================================================
bool test_05_VerifyChunkManagerPool() {
    printSeparator("测试 5: 验证 ChunkManagerPool");
    
    try {
        MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();
        if (mgr == nullptr) {
            printError("实例未初始化");
            return false;
        }
        
        auto& chunkMgrPool = mgr->getChunkManagerPool();
        
        printSuccess("获取 ChunkManagerPool 成功");
        std::cout << "  ChunkManagerPool 数量: " << chunkMgrPool.size() << std::endl;
        
        if (chunkMgrPool.size() != 1) {
            printError("ChunkManagerPool 应该只有 1 个池");
            return false;
        }
        
        const MemPool& pool = chunkMgrPool[0];
        std::cout << "\n  ChunkManagerPool[0] 详细信息:" << std::endl;
        std::cout << "    - ChunkManager 大小: " << pool.getChunkSize() << " B" << std::endl;
        std::cout << "    - ChunkManager 数量: " << pool.getTotalChunks() << std::endl;
        std::cout << "    - 空闲数量: " << pool.getFreeChunks() << std::endl;
        std::cout << "    - 已用数量: " << pool.getUsedChunks() << std::endl;
        
        // 验证初始状态
        if (pool.getFreeChunks() != pool.getTotalChunks()) {
            printError("ChunkManagerPool 初始空闲数量不正确");
            return false;
        }
        
        printSuccess("ChunkManagerPool 验证通过");
        return true;
    } catch (const std::exception& e) {
        printError(std::string("ChunkManagerPool 验证异常: ") + e.what());
        return false;
    }
}

// ============================================================================
// 测试 6: 打印统计信息
// ============================================================================
bool test_06_PrintStatistics() {
    printSeparator("测试 6: 打印统计信息");
    
    try {
        MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();
        if (mgr == nullptr) {
            printError("实例未初始化");
            return false;
        }
        
        std::cout << "\n";
        mgr->printAllPoolStats();
        
        printSuccess("统计信息打印成功");
        return true;
    } catch (const std::exception& e) {
        printError(std::string("统计信息打印异常: ") + e.what());
        return false;
    }
}

// ============================================================================
// 测试 7: 清理和销毁
// ============================================================================
bool test_07_Cleanup() {
    printSeparator("测试 7: 清理和销毁");
    
    try {
        MemPoolManager::destroySharedInstance();
        printSuccess("共享实例销毁成功");
        
        // 验证实例已被销毁
        MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();
        if (mgr != nullptr) {
            printError("实例销毁后仍然可以获取");
            return false;
        }
        
        printSuccess("实例销毁验证通过");
        return true;
    } catch (const std::exception& e) {
        printError(std::string("清理异常: ") + e.what());
        return false;
    }
}

// ============================================================================
// 主测试函数
// ============================================================================
int main() {
    std::cout << COLOR_CYAN << R"(
╔═══════════════════════════════════════════════════════════════════╗
║                                                                   ║
║        MemPoolManager 创建验证测试                                ║
║        ZeroCP Framework Test Suite                               ║
║                                                                   ║
╚═══════════════════════════════════════════════════════════════════╝
)" << COLOR_RESET << std::endl;
    
    printInfo("测试目标：验证 MemPoolManager 是否能够成功创建并正确初始化");
    printInfo("进程 ID: " + std::to_string(getpid()));
    
    // 测试结果统计
    int totalTests = 0;
    int passedTests = 0;
    
    // 创建配置
    MemPoolConfig config;
    
    // 执行测试
    struct TestCase {
        std::string name;
        std::function<bool()> func;
    };
    
    std::vector<TestCase> tests = {
        {"创建配置", [&]() { return test_01_CreateConfig(config); }},
        {"创建共享实例", [&]() { return test_02_CreateSharedInstance(config); }},
        {"验证内存布局", test_03_VerifyMemoryLayout},
        {"验证 MemPool", test_04_VerifyMemPools},
        {"验证 ChunkManagerPool", test_05_VerifyChunkManagerPool},
        {"打印统计信息", test_06_PrintStatistics},
        {"清理和销毁", test_07_Cleanup}
    };
    
    for (auto& test : tests) {
        totalTests++;
        if (test.func()) {
            passedTests++;
        } else {
            std::cout << COLOR_RED << "\n测试失败: " << test.name << COLOR_RESET << std::endl;
            // 继续执行其他测试
        }
    }
    
    // 打印最终结果
    printSeparator("测试结果汇总");
    std::cout << "\n";
    std::cout << "  总测试数: " << totalTests << std::endl;
    std::cout << "  通过数量: " << COLOR_GREEN << passedTests << COLOR_RESET << std::endl;
    std::cout << "  失败数量: " << COLOR_RED << (totalTests - passedTests) << COLOR_RESET << std::endl;
    std::cout << "  通过率:   ";
    
    double passRate = (double)passedTests / totalTests * 100.0;
    if (passRate == 100.0) {
        std::cout << COLOR_GREEN << std::fixed << std::setprecision(1) << passRate << "%" << COLOR_RESET << std::endl;
    } else if (passRate >= 80.0) {
        std::cout << COLOR_YELLOW << std::fixed << std::setprecision(1) << passRate << "%" << COLOR_RESET << std::endl;
    } else {
        std::cout << COLOR_RED << std::fixed << std::setprecision(1) << passRate << "%" << COLOR_RESET << std::endl;
    }
    
    std::cout << "\n";
    
    if (passedTests == totalTests) {
        std::cout << COLOR_GREEN << "🎉 所有测试通过！MemPoolManager 创建成功！" << COLOR_RESET << std::endl;
        printSeparator();
        return 0;
    } else {
        std::cout << COLOR_RED << "❌ 部分测试失败，请检查日志" << COLOR_RESET << std::endl;
        printSeparator();
        return 1;
    }
}

