/**
 * @file test_client.cpp
 * @brief 测试客户端 - 验证共享内存初始化
 * @details 职责：
 *   1. 连接到已存在的共享内存
 *   2. 获取 MemPoolManager 实例
 *   3. 验证内存池配置和状态
 *   4. 验证多进程访问是否成功
 */

#include "mempool_manager.hpp"
#include "mempool_config.hpp"
#include "chunk_manager.hpp"
#include "logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace ZeroCP::Memory;

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  ZeroCP 内存池管理器 - 测试客户端" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // ==================== 1. 连接到共享内存 ====================
    std::cout << "\n[1] 连接到共享内存..." << std::endl;
    
    // 客户端使用 attachToSharedInstance() 连接到服务端创建的共享内存
    // 不需要提供配置，直接读取共享内存中的配置
    bool connected = false;
    int retries = 0;
    const int maxRetries = 10;
    
    while (retries < maxRetries)
    {
        // 尝试连接到已存在的共享内存
        if (MemPoolManager::attachToSharedInstance())
        {
            std::cout << "  ✓ 成功连接到共享内存" << std::endl;
            connected = true;
            break;
        }
        
        std::cout << "  等待服务端初始化... (尝试 " << (retries + 1) << "/" << maxRetries << ")" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        retries++;
    }
    
    if (!connected)
    {
        ZEROCP_LOG(Error, "无法连接到共享内存，请确保服务端已启动");
        return 1;
    }
    
    // 获取实例
    auto* manager = MemPoolManager::getInstanceIfInitialized();
    if (!manager)
    {
        ZEROCP_LOG(Error, "连接成功但无法获取实例");
        return 1;
    }
    
    // ==================== 2. 验证内存池配置 ====================
    std::cout << "\n[2] 验证内存池配置..." << std::endl;
    std::cout << "  ✓ 成功访问到服务端创建的 MemPoolManager" << std::endl;
    std::cout << "  ✓ 多进程共享内存访问正常" << std::endl;
    
    // ==================== 3. 查看内存池状态 ====================
    std::cout << "\n[3] 客户端读取的内存池状态:" << std::endl;
    manager->printAllPoolStats();
    
    // ==================== 4. 验证数据一致性 ====================
    std::cout << "\n[4] 验证数据一致性..." << std::endl;
    std::cout << "  ℹ️  如果上面显示的内存池状态与服务端一致，" << std::endl;
    std::cout << "     说明多进程共享内存初始化成功！" << std::endl;
    std::cout << "  ℹ️  预期配置：" << std::endl;
    std::cout << "     - Pool[0]: 256B x 100 个" << std::endl;
    std::cout << "     - Pool[1]: 1KB x 50 个" << std::endl;
    std::cout << "     - Pool[2]: 4KB x 20 个" << std::endl;
    std::cout << "     - Pool[3]: 16KB x 10 个" << std::endl;
    
    // ==================== 5. 保持一段时间观察 ====================
    std::cout << "\n[5] 客户端保持运行 5 秒..." << std::endl;
    std::cout << "    （可以观察服务端的状态输出）" << std::endl;
    
    for (int i = 1; i <= 5; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "  " << i << " 秒..." << std::endl;
    }
    
    // ==================== 6. 再次查看状态 ====================
    std::cout << "\n[6] 最终内存池状态:" << std::endl;
    manager->printAllPoolStats();
    
    // 清理：客户端退出前应该断开连接
    std::cout << "\n[7] 断开共享内存连接..." << std::endl;
    MemPoolManager::destroySharedInstance();
    std::cout << "  ✓ 已断开连接" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  客户端测试完成" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}

