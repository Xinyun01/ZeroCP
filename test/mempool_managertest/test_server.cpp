/**
 * @file test_server.cpp
 * @brief 测试服务端 - 初始化共享内存池
 * @details 职责：
 *   1. 创建并初始化 MemPoolManager
 *   2. 配置内存池（不同大小的 chunk）
 *   3. 保持运行，等待客户端访问
 */

#include "mempool_manager.hpp"
#include "mempool_config.hpp"
#include "chunk_manager.hpp"
#include "logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

using namespace ZeroCP::Memory;

// 全局标志，用于优雅退出
std::atomic<bool> g_running{true};

void signalHandler(int signum)
{
    std::cout << "\n收到信号 " << signum << "，准备退出..." << std::endl;
    g_running = false;
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  ZeroCP 内存池管理器 - 测试服务端" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 注册信号处理器
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // ==================== 1. 创建内存池配置 ====================
    std::cout << "\n[1] 创建内存池配置..." << std::endl;
    
    MemPoolConfig config;
    
    // 添加不同大小的内存池
    // 小块内存：256 字节 x 100 个
    config.addMemPoolEntry(256, 100);
    std::cout << "  - 添加内存池: 256 字节 x 100 个" << std::endl;
    
    // 中等内存：1KB x 50 个
    config.addMemPoolEntry(1024, 50);
    std::cout << "  - 添加内存池: 1KB x 50 个" << std::endl;
    
    // 大块内存：4KB x 20 个
    config.addMemPoolEntry(4096, 20);
    std::cout << "  - 添加内存池: 4KB x 20 个" << std::endl;
    
    // 超大内存：16KB x 10 个
    config.addMemPoolEntry(16384, 10);
    std::cout << "  - 添加内存池: 16KB x 10 个" << std::endl;
    
    config.getMemPoolConfigInfo();
    
    // ==================== 2. 创建共享内存实例 ====================
    std::cout << "\n[2] 创建共享内存实例..." << std::endl;
    
    if (!MemPoolManager::createSharedInstance(config))
    {
        ZEROCP_LOG(Error, "创建共享内存实例失败");
        return 1;
    }
    
    std::cout << "  ✓ 共享内存实例创建成功" << std::endl;
    
    // ==================== 3. 获取实例并验证 ====================
    std::cout << "\n[3] 获取并验证实例..." << std::endl;
    
    auto* manager = MemPoolManager::getInstanceIfInitialized();
    if (!manager)
    {
        ZEROCP_LOG(Error, "获取 MemPoolManager 实例失败");
        return 1;
    }
    
    std::cout << "  ✓ MemPoolManager 实例获取成功" << std::endl;
    
    // ==================== 4. 打印内存池状态 ====================
    std::cout << "\n[4] 内存池初始状态:" << std::endl;
    manager->printAllPoolStats();
    
    // ==================== 5. 测试 chunk 分配 ====================
    std::cout << "\n[5] 测试 chunk 分配..." << std::endl;
    
    // 测试分配不同大小的 chunk
    ChunkManager* chunk1 = manager->getChunk(100);  // 应该从 256B 池分配
    if (chunk1)
    {
        std::cout << "  ✓ 成功分配 100 字节 chunk" << std::endl;
    }
    else
    {
        std::cout << "  ✗ 分配 100 字节 chunk 失败" << std::endl;
    }
    
    ChunkManager* chunk2 = manager->getChunk(512);  // 应该从 1KB 池分配
    if (chunk2)
    {
        std::cout << "  ✓ 成功分配 512 字节 chunk" << std::endl;
    }
    else
    {
        std::cout << "  ✗ 分配 512 字节 chunk 失败" << std::endl;
    }
    
    ChunkManager* chunk3 = manager->getChunk(2048);  // 应该从 4KB 池分配
    if (chunk3)
    {
        std::cout << "  ✓ 成功分配 2048 字节 chunk" << std::endl;
    }
    else
    {
        std::cout << "  ✗ 分配 2048 字节 chunk 失败" << std::endl;
    }
    
    // 打印分配后的内存池状态
    std::cout << "\n[6] 分配后的内存池状态:" << std::endl;
    manager->printAllPoolStats();
    
    // ==================== 7. 保持运行，等待客户端 ====================
    std::cout << "\n========================================" << std::endl;
    std::cout << "  服务端运行中..." << std::endl;
    std::cout << "  按 Ctrl+C 退出" << std::endl;
    std::cout << "  客户端可以连接并访问共享内存" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int seconds = 0;
    while (g_running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        seconds += 5;
        
        std::cout << "\n[运行时间: " << seconds << "秒]" << std::endl;
        manager->printAllPoolStats();
    }
    
    // ==================== 8. 清理资源 ====================
    std::cout << "\n[8] 清理资源..." << std::endl;
    
    // 销毁共享实例
    MemPoolManager::destroySharedInstance();
    std::cout << "  ✓ 共享实例已销毁" << std::endl;
    
    std::cout << "\n服务端退出成功" << std::endl;
    return 0;
}

