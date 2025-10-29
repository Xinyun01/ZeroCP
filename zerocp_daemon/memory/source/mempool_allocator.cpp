#include "mempool_allocator.hpp"
#include "mempool_manager.hpp"
#include "mempool.hpp"
#include "mempool_config.hpp"
#include "chunk_manager.hpp"
#include "bump_allocator.hpp"
#include "mpmclockfreelist.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "zerocp_foundationLib/memory/include/memory.hpp"

using ZeroCP::Memory::align;

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
                                           const Perms permissions
                                           const uint64_t sharedMemorySize) noexcept
{
    // 获取 MemPoolManager 实例以计算总内存大小 

    m_shmProvider = std::make_unique<PosixShmProvider>(
        name,
        sharedMemorySize,
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
bool MemPoolAllocator::layoutMemory(void* baseAddress,uint64_t memorySize) noexcept
{
    if (baseAddress == nullptr || memorySize == 0)
    {
        ZEROCP_LOG(Error, "Invalid parameters: baseAddress={}, memorySize={}", 
                   static_cast<void*>(baseAddress), memorySize);
        return false;
    }
    
    // 获取 MemPoolManager 实例
    auto& manager = MemPoolManager::getInstance(m_config);
    // 获取内存池列表和配置
    auto& chunkManagerPool = manager.getChunkManagerPool();
    auto& mempools = manager.getMemPools();
    const auto& entries = m_config.m_memPoolEntries;

    // 使用 BumpAllocator 从管理内存中分配
    BumpAllocator allocator(baseAddress, memorySize);
 
    // 为每个配置的内存池分配并初始化 MemPool 对象
    auto totalchunknums = 0;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        const auto& entry = entries[i];
        totalchunknums += entry.m_poolCount;
        // 在共享内存中为 MemPool 对象分配内存
        auto poolMemoryResult = allocator.allocate(sizeof(MemPool), 8U);
        if (!poolMemoryResult.has_value())
        {
            ZEROCP_LOG(Error, "Failed to allocate memory for MemPool[{}]", i);
            return false;
        }
        void* poolMemory = poolMemoryResult.value();
        
        // 使用 placement new 在共享内存中构造 MemPool 对象
        MemPool* pool = new (poolMemory) MemPool();
        
        // 为每个池计算需要的 freeList 内存大小
        // 注意：根据 mempool_manager.cpp 的用法，使用实例方法调用
        Concurrent::MPMC_LockFree_List tempList(nullptr, 0);
        uint64_t freeListSize = tempList.requiredIndexMemorySize(entry.m_poolCount);
        freeListSize = align(freeListSize, 8U);
        
        // 分配 freeList 内存
        auto freeListResult = allocator.allocate(freeListSize, 8U);
        if (!freeListResult.has_value())
        {
            ZEROCP_LOG(Error, "Failed to allocate freeList memory for MemPool[{}]", i);
            return false;
        }
        void* freeListMemory = freeListResult.value();
        
        // 初始化 MemPool
        // 注意：rawMemory 需要从 chunk 内存区域分配，这里暂时传入 nullptr
        // 实际应该在 layoutChunkMemory 中设置
        if (!pool->initialize(baseAddress, nullptr, entry.m_poolSize, entry.m_poolCount, freeListMemory, static_cast<uint64_t>(i)))
        {
            ZEROCP_LOG(Error, "Failed to initialize MemPool[{}]", i);
            return false;
        }
        //push_back 会调用拷贝构造函数，emplace_back 会调用移动构造函数
        mempools.emplace_back(*pool);
    }

    // 分配 chunkManager 管理对象内存池
    // chunkManagerPool 是一个 MemPool，用于管理所有的 ChunkManager 对象
    // 需要分配：1. ChunkManager 对象内存 2. 静态链表（用于 MemPool 的空闲索引管理）
    
    // 1. 为 chunkManager 对象分配内存（这些对象将被 MemPool 管理）
    auto chunkManagerMemoryResult = allocator.allocate(totalchunknums * sizeof(ChunkManager), 8U);
    if (!chunkManagerMemoryResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to allocate chunkManager memory");
        return false;
    }
    void* chunkManagerMemory = chunkManagerMemoryResult.value();
    
    // 2. 为 MemPool 的静态链表分配内存（用于管理空闲索引）
    Concurrent::MPMC_LockFree_List tempListForChunkManager(nullptr, 0);
    uint64_t chunkManagerFreeListSize = tempListForChunkManager.requiredIndexMemorySize(static_cast<uint32_t>(totalchunknums));
    chunkManagerFreeListSize = align(chunkManagerFreeListSize, 8U);
    
    auto chunkManagerFreeListResult = allocator.allocate(chunkManagerFreeListSize, 8U);
    if (!chunkManagerFreeListResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to allocate freeList memory for chunkManager MemPool");
        return false;
    }
    void* chunkManagerFreeListMemory = chunkManagerFreeListResult.value();
    
    // 3. 在共享内存中为 MemPool 对象分配内存
    auto chunkManagerPoolMemPoolResult = allocator.allocate(sizeof(MemPool), 8U);
    if (!chunkManagerPoolMemPoolResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to allocate memory for chunkManager MemPool");
        return false;
    }
    void* chunkManagerPoolMemPoolMemory = chunkManagerPoolMemPoolResult.value();
    
    // 4. 使用 placement new 在共享内存中构造 MemPool 对象
    MemPool* chunkManagerMemPool = new (chunkManagerPoolMemPoolMemory) MemPool();
    
    // 5. 初始化 chunkManager 的 MemPool
    // 参数说明：
    // - baseAddress: 共享内存基地址
    // - chunkManagerMemory: ChunkManager 对象数组的内存（rawMemory）
    // - sizeof(ChunkManager): 每个 chunk 的大小（即每个 ChunkManager 对象的大小）
    // - totalchunknums: chunk 的数量（即 ChunkManager 对象的数量）
    // - chunkManagerFreeListMemory: 静态链表的内存（用于管理空闲索引）
    // - 0: pool_id
    if (!chunkManagerMemPool->initialize(baseAddress, chunkManagerMemory, sizeof(ChunkManager), static_cast<uint32_t>(totalchunknums), chunkManagerFreeListMemory, 0))
    {
        ZEROCP_LOG(Error, "Failed to initialize chunkManager MemPool");
        return false;
    }
    
    // 6. 将 MemPool 添加到 chunkManagerPool vector 中
    // chunkManagerPool 是 MemPoolManager 的成员变量，直接添加即可
    chunkManagerPool.emplace_back(*chunkManagerMemPool);
   
  
    return true;
}


} // namespace Memory
} // namespace ZeroCP

