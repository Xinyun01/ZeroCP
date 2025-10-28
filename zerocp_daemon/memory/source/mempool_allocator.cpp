#include "mempool_allocator.hpp"
#include "mempool_manager.hpp"
#include "mempool.hpp"
#include "mempool_config.hpp"
#include "chunk_manager.hpp"
#include "bump_allocator.hpp"
#include "mpmclockfreelist.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"

namespace ZeroCP
{
namespace Memory
{

MemPoolAllocator::MemPoolAllocator(const MemPoolConfig& config) noexcept
    : m_config(config)
{
}

void* MemPoolAllocator::createSharedMemory(const Name_t& name,
                                           const AccessMode accessMode,
                                           const OpenMode openMode,
                                           const Perms permissions) noexcept
{
    // 获取 MemPoolManager 实例以计算总内存大小
    auto& manager = MemPoolManager::getInstance(m_config);
    m_totalMemorySize = manager.getTotalMemorySize();
    
    // 创建 PosixShmProvider
    m_shmProvider = std::make_unique<PosixShmProvider>(
        name,
        m_totalMemorySize,
        accessMode,
        openMode,
        permissions
    );
    
    // 创建共享内存并获取基地址
    auto result = m_shmProvider->createMemory();
    if (!result.has_value())
    {
        ZEROCP_LOG(Error, "Failed to create shared memory");
        return nullptr;
    }
    
    m_baseAddress = result.value();
    return m_baseAddress;
}
//这里需要传入共享内存的映射地址和需要的总大小
bool MemPoolAllocator::layoutMemory(void* baseAddress, uint64_t totalSize) noexcept
{
    if (baseAddress == nullptr || totalSize == 0)
    {
        ZEROCP_LOG(Error, "Invalid parameters: baseAddress={}, totalSize={}", 
                   static_cast<void*>(baseAddress), totalSize);
        return false;
    }
    
    // 获取 MemPoolManager 实例
    auto& manager = MemPoolManager::getInstance(m_config);
    
    // 获取内存池列表和配置
    auto& chunkManagerPool = manager.getChunkManagerPool();
    auto& mempools = manager.getMemPools();
    const auto& entries = m_config.m_memPoolEntries;
    
    // 创建 BumpAllocator 用于内存分配
    BumpAllocator allocator(baseAddress, totalSize);
    
    // ============ 第一步：初始化数据 chunk 内存池 ============
    ZEROCP_LOG(Info, "Initializing {} data chunk memory pools", entries.size());
    
    for (uint64_t i = 0; i < entries.size() && i < mempools.size(); ++i)
    {
        const auto& entry = entries[i];
        auto& pool = mempools[i];
                
        // 1. 为空闲索引链表分配内存
        uint64_t freeListSize = Concurrent::MPMC_LockFree_List::requiredIndexMemorySize(entry.m_poolCount);
        void* freeListMemory = allocator.allocate(freeListSize, 8);
        if (freeListMemory == nullptr)
        {
            ZEROCP_LOG(Error, "Failed to allocate free list memory for pool {}: size={}", 
                       i, freeListSize);
            return false;
        }
        // 2. 为 chunk 数据分配内存 = chunkheadsize+usrloadSize
        uint64_t chunkDataSize = sizeof(ChunkHead) * entry.m_poolCount + entry.m_usrloadSize * entry.m_poolCount;
        void* chunkMemory = allocator.allocate(chunkDataSize, 8);
        if (chunkMemory == nullptr)
        {
            ZEROCP_LOG(Error, "Failed to allocate chunk memory for pool {}: size={}", 
                       i, chunkDataSize);
            return false;
        } 
        // 3. 初始化内存池 传入当前进程
        if (!pool.initialize(baseAddress,chunkMemory, entry.m_poolSize+ sizeof(ChunkHeader), entry.m_poolCount, freeListMemory, i))
        {
            ZEROCP_LOG(Error, "Failed to initialize memory pool {}", i);
            return false;
        }
        
    }
    
    // ============ 第二步：初始化 ChunkManager 对象池 ============
    ZEROCP_LOG(Info, "Initializing ChunkManager object pool");
    
    // 计算所有 chunk 的总数（即需要的 ChunkManager 对象数量）
    uint32_t totalChunkCount = 0;
    for (const auto& entry : entries)
    {
        totalChunkCount += entry.m_poolCount;
    }
    
    if (!chunkManagerPool.empty())
    {
        auto& mgmtPool = chunkManagerPool[0];
        
        // 1. 为 ChunkManager 对象分配内存
        uint64_t chunkManagerSize = sizeof(ChunkManager) * totalChunkCount;
        void* chunkManagerMemory = allocator.allocate(chunkManagerSize, 8);
        if (chunkManagerMemory == nullptr)
        {
            ZEROCP_LOG(Error, "Failed to allocate ChunkManager memory: size={}", chunkManagerSize);
            return false;
        }
        
        // 2. 为 ChunkManager 池的空闲索引链表分配内存
        uint64_t mgmtFreeListSize = Concurrent::MPMC_LockFree_List::requiredIndexMemorySize(totalChunkCount);
        void* mgmtFreeListMemory = allocator.allocate(mgmtFreeListSize, 8);
        if (mgmtFreeListMemory == nullptr)
        {
            ZEROCP_LOG(Error, "Failed to allocate ChunkManager free list memory: size={}", mgmtFreeListSize);
            return false;
        }
        
        // 3. 初始化 ChunkManager 对象池
        if (!mgmtPool.initialize(baseAddress,chunkManagerMemory, sizeof(ChunkManager), totalChunkCount, mgmtFreeListMemory))
        {
            ZEROCP_LOG(Error, "Failed to initialize ChunkManager pool");
            return false;
        }
        
        ZEROCP_LOG(Debug, "ChunkManager pool initialized: objectSize={}, objectCount={}, dataMemory={}, freeListMemory={}", 
                   sizeof(ChunkManager), totalChunkCount, chunkManagerMemory, mgmtFreeListMemory);
    }
    
    // ============ 第三步：验证内存使用 ============
    uint64_t usedMemory = allocator.getUsedSize();
    ZEROCP_LOG(Info, "Memory layout completed: {} data pools, 1 management pool, used {}/{} bytes", 
               entries.size(), usedMemory, totalSize);
    
    return true;
}

} // namespace Memory
} // namespace ZeroCP

