/**
 * @file test_chunk_reader.cpp
 * @brief 跨进程 Chunk 读取测试 - 读进程
 * @details 
 *   职责：
 *   1. 附加到已存在的共享内存池
 *   2. 从内存池 1/2/3 读取第一个 Chunk
 *   3. 验证数据完整性
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
#include <unistd.h>  // for getpid()

using namespace ZeroCP::Memory;

// 测试数据结构（必须与写进程一致）
struct TestData
{
    uint64_t magic;        // 魔数
    uint32_t poolId;       // 池ID
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

// 从指定池读取并验证第一个 Chunk
// 注意：这里简化处理，通过分配相同大小的chunk来访问写进程创建的数据
bool readAndVerifyChunkFromPool(MemPoolManager* manager, uint32_t poolId, uint32_t expectedSequence)
{
    std::cout << "\n========== 从内存池 " << poolId << " 读取数据 ==========" << std::endl;
    
    // 获取对应池的chunk大小
    auto& pools = manager->getMemPools();
    if (poolId >= pools.size())
    {
        std::cout << "  ✗ 池ID " << poolId << " 超出范围" << std::endl;
        return false;
    }
    
    size_t dataSize = pools[poolId].getChunkSize();
    
    // 分配一个chunk - 这会获取写进程分配的第一个chunk（因为空闲链表是FIFO）
    // 注意：这种方法假设写进程按顺序分配，读进程也按顺序获取
    ChunkManager* targetChunk = manager->getChunk(dataSize);
    
    if (!targetChunk)
    {
        std::cout << "  ✗ 无法获取 Chunk（可能写进程未分配或已被其他进程获取）" << std::endl;
        return false;
    }
    
    std::cout << "  ✓ 成功获取 Chunk" << std::endl;
    
    // 打印 ChunkManager 信息
    std::cout << "\nChunkManager 信息:" << std::endl;
    std::cout << "  - 地址: " << static_cast<void*>(targetChunk) << std::endl;
    std::cout << "  - Chunk 索引: " << targetChunk->m_chunkIndex << std::endl;
    std::cout << "  - ChunkManager 索引: " << targetChunk->m_chunkManagerIndex << std::endl;
    std::cout << "  - 引用计数: " << targetChunk->m_refCount.load() << std::endl;
    
    // 获取 ChunkHeader
    ChunkHeader* header = targetChunk->m_chunkHeader.get();
    if (!header)
    {
        std::cout << "  ✗ 无法获取 ChunkHeader（RelativePointer 解析失败）" << std::endl;
        return false;
    }
    
    std::cout << "  ✓ ChunkHeader 地址: " << static_cast<void*>(header) << std::endl;
    
    // 获取用户数据区（使用偏移量计算）
    void* userData = reinterpret_cast<char*>(header) + header->m_userPayloadOffset;
    if (!userData)
    {
        std::cout << "  ✗ 无法获取用户数据区" << std::endl;
        return false;
    }
    
    std::cout << "  ✓ 用户数据区地址: " << userData << std::endl;
    
    // 读取并验证数据
    TestData* data = static_cast<TestData*>(userData);
    
    std::cout << "\n读取的数据:" << std::endl;
    std::cout << "  - Magic: 0x" << std::hex << data->magic << std::dec;
    if (data->magic == 0xDEADBEEF12345678ULL)
    {
        std::cout << " ✓" << std::endl;
    }
    else
    {
        std::cout << " ✗ (期望: 0xDEADBEEF12345678)" << std::endl;
        return false;
    }
    
    std::cout << "  - PoolId: " << data->poolId;
    if (data->poolId == poolId)
    {
        std::cout << " ✓" << std::endl;
    }
    else
    {
        std::cout << " ✗ (期望: " << poolId << ")" << std::endl;
        return false;
    }
    
    std::cout << "  - Sequence: " << data->sequence;
    if (data->sequence == expectedSequence)
    {
        std::cout << " ✓" << std::endl;
    }
    else
    {
        std::cout << " ✗ (期望: " << expectedSequence << ")" << std::endl;
        return false;
    }
    
    // 验证校验和
    uint32_t expectedChecksum = data->calculateChecksum();
    std::cout << "  - Checksum: 0x" << std::hex << data->checksum << std::dec;
    if (data->checksum == expectedChecksum)
    {
        std::cout << " ✓" << std::endl;
    }
    else
    {
        std::cout << " ✗ (期望: 0x" << std::hex << expectedChecksum << std::dec << ")" << std::endl;
        return false;
    }
    
    std::cout << "  - Message: " << data->message << std::endl;
    
    std::cout << "\n  ✓✓✓ 池 " << poolId << " 数据验证成功！✓✓✓" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return true;
}

int main()
{
    std::cout << "\n\n" << std::endl;
    std::cout << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║   跨进程 Chunk 测试 - 读进程          ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    std::cout << "进程ID: " << getpid() << std::endl;
    std::cout << "时间: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
    
    // 等待用户确认写进程已启动
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "⚠️  请确保写进程已经启动并完成初始化" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "按 Enter 继续..." << std::endl;
    std::cin.get();
    
    // ==================== 1. 附加到共享内存 ====================
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "[步骤 1] 附加到已存在的共享内存" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::cout << "正在附加到共享内存..." << std::endl;
    if (!MemPoolManager::attachToSharedInstance())
    {
        ZEROCP_LOG(Error, "附加到共享内存失败");
        std::cout << "\n❌ 附加失败！可能的原因：" << std::endl;
        std::cout << "  1. 写进程尚未启动" << std::endl;
        std::cout << "  2. 共享内存名称不匹配" << std::endl;
        std::cout << "  3. 权限不足" << std::endl;
        return 1;
    }
    
    std::cout << "✓ 成功附加到共享内存" << std::endl;
    
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
    std::cout << "[步骤 2] 读进程视角的内存池状态" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    manager->printAllPoolStats();
    
    // ==================== 3. 从池 1/2/3 读取并验证数据 ====================
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "[步骤 3] 从内存池 1/2/3 读取并验证数据" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    bool allSuccess = true;
    
    // 从池1读取数据
    if (!readAndVerifyChunkFromPool(manager, 1, 101))
    {
        std::cout << "  ✗ 池1数据验证失败" << std::endl;
        allSuccess = false;
    }
    
    // 从池2读取数据
    if (!readAndVerifyChunkFromPool(manager, 2, 202))
    {
        std::cout << "  ✗ 池2数据验证失败" << std::endl;
        allSuccess = false;
    }
    
    // 从池3读取数据
    if (!readAndVerifyChunkFromPool(manager, 3, 303))
    {
        std::cout << "  ✗ 池3数据验证失败" << std::endl;
        allSuccess = false;
    }
    
    // ==================== 4. 测试结果总结 ====================
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "[步骤 4] 测试结果总结" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    if (allSuccess)
    {
        std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║   ✓✓✓ 所有测试通过！✓✓✓            ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl;
        std::cout << "\n✅ 成功验证：" << std::endl;
        std::cout << "  1. ✓ 跨进程共享内存附加成功" << std::endl;
        std::cout << "  2. ✓ 从池1/2/3读取数据成功" << std::endl;
        std::cout << "  3. ✓ RelativePointer 地址转换正确" << std::endl;
        std::cout << "  4. ✓ 数据完整性验证通过（魔数、池ID、序列号、校验和）" << std::endl;
        std::cout << "  5. ✓ 跨进程零拷贝访问成功" << std::endl;
        std::cout << "\n🎉 测试结论：ZeroCP 框架跨进程零拷贝功能正常！" << std::endl;
    }
    else
    {
        std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║   ✗✗✗ 部分测试失败 ✗✗✗             ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl;
        std::cout << "\n❌ 请检查上述日志以获取详细错误信息" << std::endl;
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    // ==================== 5. 清理 ====================
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "[步骤 5] 清理资源" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    MemPoolManager::destroySharedInstance();
    std::cout << "✓ 资源清理完成" << std::endl;
    std::cout << "\n💡 提示：现在可以在写进程终端按 Ctrl+C 退出写进程" << std::endl;
    
    return allSuccess ? 0 : 1;
}
