/**
 * @file test_chunk_writer.cpp
 * @brief 跨进程 Chunk 写入测试 - 写进程
 * @details 
 *   职责：
 *   1. 创建共享内存池
 *   2. 向内存池 1/2/3 各写入一个 Chunk
 *   3. 保持运行，等待读进程验证
 */

#include "mempool_manager.hpp"
#include "mempool_config.hpp"
#include "chunk_manager.hpp"
#include "chunk_header.hpp"
#include "logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <vector>
#include <unistd.h>  // for getpid()

using namespace ZeroCP::Memory;

// 测试数据结构
struct TestData
{
    uint64_t magic;        // 魔数: 0xDEADBEEF12345678
    uint32_t poolId;       // 池ID (1/2/3)
    uint32_t sequence;     // 序列号
    uint32_t checksum;     // 校验和
    char message[240];     // 测试消息
    
    // 计算校验和
    uint32_t calculateChecksum() const
    {
        uint32_t sum = 0;
        sum += static_cast<uint32_t>(magic);
        sum += static_cast<uint32_t>(magic >> 32);
        sum += poolId;
        sum += sequence;
        for (size_t i = 0; i < sizeof(message); ++i)
        {
            sum += static_cast<uint8_t>(message[i]);
        }
        return sum;
    }
};

// 向指定内存池写入测试数据
bool writeTestDataToPool(MemPoolManager* manager, uint32_t poolId, uint32_t sequence)
{
    std::cout << "\n========== 向内存池 " << poolId << " 写入数据 ==========" << std::endl;
    
    // 获取对应池的chunk大小
    auto& pools = manager->getMemPools();
    if (poolId >= pools.size())
    {
        std::cout << "  ✗ 池ID " << poolId << " 超出范围" << std::endl;
        return false;
    }
    
    size_t dataSize = pools[poolId].getChunkSize();
    
    // 分配 Chunk
    ChunkManager* chunk = manager->getChunk(dataSize);
    
    if (!chunk)
    {
        std::cout << "  ✗ 从池 " << poolId << " 分配 Chunk 失败" << std::endl;
        return false;
    }
    
    std::cout << "  ✓ Chunk 分配成功" << std::endl;
    std::cout << "    ChunkManager 地址: " << static_cast<void*>(chunk) << std::endl;
    std::cout << "    Chunk 索引: " << chunk->m_chunkIndex << std::endl;
    std::cout << "    ChunkManager 索引: " << chunk->m_chunkManagerIndex << std::endl;
    
    // 获取 ChunkHeader
    ChunkHeader* header = chunk->m_chunkHeader.get();
    if (!header)
    {
        std::cout << "  ✗ 无法获取 ChunkHeader" << std::endl;
        return false;
    }
    
    std::cout << "  ✓ ChunkHeader 获取成功" << std::endl;
    std::cout << "    ChunkHeader 地址: " << static_cast<void*>(header) << std::endl;
    
    // 获取用户数据区（使用偏移量计算）
    void* userData = reinterpret_cast<char*>(header) + header->m_userPayloadOffset;
    if (!userData)
    {
        std::cout << "  ✗ 无法获取用户数据区" << std::endl;
        return false;
    }
    
    std::cout << "  ✓ 用户数据区获取成功" << std::endl;
    std::cout << "    用户数据区地址: " << userData << std::endl;
    
    // 构造测试数据
    TestData* data = static_cast<TestData*>(userData);
    data->magic = 0xDEADBEEF12345678ULL;
    data->poolId = poolId;
    data->sequence = sequence;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Test data from Pool %u, Sequence %u", poolId, sequence);
    strncpy(data->message, msg, sizeof(data->message) - 1);
    data->message[sizeof(data->message) - 1] = '\0';
    
    // 计算并设置校验和
    data->checksum = data->calculateChecksum();
    
    std::cout << "\n写入的数据:" << std::endl;
    std::cout << "  - Magic: 0x" << std::hex << data->magic << std::dec << std::endl;
    std::cout << "  - PoolId: " << data->poolId << std::endl;
    std::cout << "  - Sequence: " << data->sequence << std::endl;
    std::cout << "  - Checksum: 0x" << std::hex << data->checksum << std::dec << std::endl;
    std::cout << "  - Message: " << data->message << std::endl;
    
    std::cout << "\n  ✓✓✓ 池 " << poolId << " 数据写入成功！✓✓✓" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return true;
}

int main()
{
    std::cout << "\n\n" << std::endl;
    std::cout << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   跨进程 Chunk 测试 - 写进程          ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    std::cout << "进程ID: " << getpid() << std::endl;
    std::cout << "时间: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
    
    // ==================== 1. 创建共享内存池 ====================
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "[步骤 1] 创建共享内存池" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    // 配置内存池
    MemPoolConfig config;
    config.addMemPoolEntry(128,   100);   // 池0: 128B × 100
    config.addMemPoolEntry(1024,  50);    // 池1: 1KB × 50
    config.addMemPoolEntry(4096,  20);    // 池2: 4KB × 20
    config.addMemPoolEntry(16384, 10);    // 池3: 16KB × 10
    
    std::cout << "内存池配置:" << std::endl;
    for (size_t i = 0; i < config.m_memPoolEntries.size(); ++i)
    {
        std::cout << "  池" << i << ": " << config.m_memPoolEntries[i].m_chunkSize << " 字节 × " 
                  << config.m_memPoolEntries[i].m_chunkCount << " 个 = " 
                  << (config.m_memPoolEntries[i].m_chunkSize * config.m_memPoolEntries[i].m_chunkCount / 1024.0) << " KB" << std::endl;
    }
    
    std::cout << "\n正在创建共享内存..." << std::endl;
    if (!MemPoolManager::createSharedInstance(config))
    {
        ZEROCP_LOG(Error, "创建共享内存池失败");
        return 1;
    }
    
    std::cout << "✓ 共享内存池创建成功" << std::endl;
    
    // 获取实例
    auto* manager = MemPoolManager::getInstanceIfInitialized();
    if (!manager)
    {
        ZEROCP_LOG(Error, "获取 MemPoolManager 实例失败");
        return 1;
    }
    
    std::cout << "✓ MemPoolManager 实例获取成功" << std::endl;
    std::cout << "  实例地址: " << static_cast<void*>(manager) << std::endl;
    
    // ==================== 2. 打印内存池状态 ====================
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "[步骤 2] 写进程初始化后的内存池状态" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    manager->printAllPoolStats();
    
    // ==================== 3. 向池 1/2/3 写入测试数据 ====================
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "[步骤 3] 向内存池 1/2/3 写入测试数据" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    bool allSuccess = true;
    
    // 向池1写入数据 (1KB)
    if (!writeTestDataToPool(manager, 1, 101))
    {
        std::cout << "  ✗ 池1写入失败" << std::endl;
        allSuccess = false;
    }
    
    // 向池2写入数据 (4KB)
    if (!writeTestDataToPool(manager, 2, 202))
    {
        std::cout << "  ✗ 池2写入失败" << std::endl;
        allSuccess = false;
    }
    
    // 向池3写入数据 (16KB)
    if (!writeTestDataToPool(manager, 3, 303))
    {
        std::cout << "  ✗ 池3写入失败" << std::endl;
        allSuccess = false;
    }
    
    // ==================== 4. 打印写入后的内存池状态 ====================
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "[步骤 4] 写入数据后的内存池状态" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    manager->printAllPoolStats();
    
    // ==================== 5. 等待读进程验证 ====================
    if (allSuccess)
    {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "[步骤 5] 等待读进程验证" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║   ✓✓✓ 数据写入完成！✓✓✓             ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl;
        std::cout << "\n📝 写进程状态：" << std::endl;
        std::cout << "  • 进程ID: " << getpid() << std::endl;
        std::cout << "  • 共享内存已创建并写入数据" << std::endl;
        std::cout << "  • 池1: 已写入 sequence=101" << std::endl;
        std::cout << "  • 池2: 已写入 sequence=202" << std::endl;
        std::cout << "  • 池3: 已写入 sequence=303" << std::endl;
        std::cout << "\n📌 下一步操作：" << std::endl;
        std::cout << "  1. 保持此终端运行" << std::endl;
        std::cout << "  2. 在另一个终端启动读进程：./test_chunk_reader" << std::endl;
        std::cout << "  3. 验证完成后，按 Ctrl+C 退出写进程" << std::endl;
        std::cout << "\n⏳ 写进程保持运行中..." << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        
        // 保持运行
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    else
    {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  ✗✗✗ 数据写入失败 ✗✗✗" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // 清理
        MemPoolManager::destroySharedInstance();
        return 1;
    }
    
    return 0;
}
