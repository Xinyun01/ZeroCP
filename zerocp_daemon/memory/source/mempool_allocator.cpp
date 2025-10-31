#include "mempool_allocator.hpp"
#include "mempool_manager.hpp"
#include "mempool.hpp"
#include "mempool_config.hpp"
#include "chunk_manager.hpp"
#include "chunk_header.hpp"
#include "bump_allocator.hpp"
#include "mpmclockfreelist.hpp"
#include "memory.hpp"
#include "logging.hpp"
#include <cstring>

using ZeroCP::Memory::align;

namespace ZeroCP
{
namespace Memory
{

// ==================== 构造函数 ====================

MemPoolAllocator::MemPoolAllocator(const MemPoolConfig& config, void* sharedMemoryBase) noexcept
    : m_config(config)
    , m_sharedMemoryBase(sharedMemoryBase)
{
    // 配置引用必须有效（由调用者保证）
}

// ==================== 共享内存创建 ====================
// 注意：共享内存创建已移至 MemPoolManager::initialize()
// 此方法已废弃，不再使用

/* 已废弃
void* MemPoolAllocator::createSharedMemory(const Name_t& name,
                                           const uint64_t sharedMemorySize,
                                           const AccessMode accessMode,
                                           const OpenMode openMode,
                                           const Perms permissions) noexcept
{
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
*/

// ==================== 管理区内存布局 ====================

bool MemPoolAllocator::ManagementMemoryLayout(void* mgmtBaseAddress, uint64_t mgmtMemorySize,
                                              vector<MemPool, 16>& mempools,
                                              vector<MemPool, 1>& chunkManagerPool) noexcept
{
    if (mgmtBaseAddress == nullptr || mgmtMemorySize == 0)
    {
        ZEROCP_LOG(Error, "Invalid parameters");
        return false;
    }

    // 使用 BumpAllocator 从管理内存中分配
    BumpAllocator allocator(mgmtBaseAddress, mgmtMemorySize);
    
    uint64_t totalChunks = 0;
    uint64_t poolIndex = 0;
    
    // 1. 为每个内存池分配 freeList 内存，并使用 emplace_back 在共享内存中直接构造 MemPool
    for (uint64_t i = 0; i < m_config.m_memPoolEntries.size(); ++i)
    {
        const auto& entry = m_config.m_memPoolEntries[i];
        
        // 1.1 计算 freeList 大小
        uint64_t freeListSize = align(
            Concurrent::MPMC_LockFree_List::requiredIndexMemorySize(entry.m_chunkCount), 8U);
        
        // 1.2 分配 freeList 内存
        auto freeListResult = allocator.allocate(freeListSize, 8U);
        if (!freeListResult.has_value())
        {
            ZEROCP_LOG(Error, "Failed to allocate freeList memory for Pool " << poolIndex);
            return false;
        }
        void* freeListMemory = freeListResult.value();
        
        // 1.3 使用 emplace_back 在共享内存中的 vector 里直接构造 MemPool 对象
        // 注意：rawMemory 在 ChunkMemoryLayout 中设置
        bool success = mempools.emplace_back(
            m_sharedMemoryBase,       // baseAddress
            nullptr,                  // rawMemory (稍后设置)
            entry.m_chunkSize,        // chunkSize
            entry.m_chunkCount,       // chunkNums
            freeListMemory,           // freeListMemory
            poolIndex                 // pool_id
        );
        
        if (!success)
        {
            ZEROCP_LOG(Error, "Failed to add MemPool to vector (capacity exceeded)");
            return false;
        }
        
        totalChunks += entry.m_chunkCount;
        poolIndex++;
    }
    
    // 2. 分配所有 ChunkManager 对象的内存
    uint64_t chunkManagerArraySize = totalChunks * align(sizeof(ChunkManager), 8U);
    auto chunkManagerResult = allocator.allocate(chunkManagerArraySize, 8U);
    if (!chunkManagerResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to allocate ChunkManager array");
        return false;
    }
    void* chunkManagerMemory = chunkManagerResult.value();
    
    // 3. 分配 ChunkManagerPool 的 freeList 内存
    uint64_t chunkMgrFreeListSize = align(
        Concurrent::MPMC_LockFree_List::requiredIndexMemorySize(totalChunks), 8U);
    auto chunkMgrFreeListResult = allocator.allocate(chunkMgrFreeListSize, 8U);
    if (!chunkMgrFreeListResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to allocate ChunkManagerPool freeList");
        return false;
    }
    void* chunkMgrFreeListMemory = chunkMgrFreeListResult.value();
    
    // 4. 使用 emplace_back 在共享内存中构造 ChunkManagerPool（用于管理 ChunkManager 对象的回收）
    bool success = chunkManagerPool.emplace_back(
        m_sharedMemoryBase,           // baseAddress
        chunkManagerMemory,           // rawMemory (ChunkManager 对象数组)
        align(sizeof(ChunkManager), 8U),  // chunkSize
        static_cast<uint32_t>(totalChunks),  // chunkNums
        chunkMgrFreeListMemory,       // freeListMemory
        0UL                           // pool_id
    );
    
    if (!success)
    {
        ZEROCP_LOG(Error, "Failed to create ChunkManagerPool");
        return false;
    }
    
    ZEROCP_LOG(Info, "ManagementMemoryLayout completed successfully");
    return true;
}

// ==================== 数据区内存布局 ====================

bool MemPoolAllocator::ChunkMemoryLayout(void* baseAddress, uint64_t memorySize,
                                         vector<MemPool, 16>& mempools) noexcept
{
    if (baseAddress == nullptr || memorySize == 0)
    {
        ZEROCP_LOG(Error, "Invalid parameters");
        return false;
    }
    
    // 使用 BumpAllocator 从数据内存中分配
    BumpAllocator allocator(baseAddress, memorySize);
    
    // 按照 memory.md 第80-117行的布局：
    // 数据区 = [池0的Chunk数组] [池1的Chunk数组] [池2的Chunk数组] ...
    // 为每个 MemPool 分配 chunk 数据块并记录偏移量
    for (uint64_t i = 0; i < mempools.size(); ++i)
    {
        MemPool& pool = mempools[i];
        const auto& entry = m_config.m_memPoolEntries[i];
        
        // 每个 chunk 的实际大小 = ChunkHeader + 用户数据
        uint64_t actualChunkSize = align(sizeof(ChunkHeader) + entry.m_chunkSize, 8U);
        
        // 计算这个 Pool 需要的总内存
        uint64_t poolTotalSize = actualChunkSize * entry.m_chunkCount;
        
        // 分配 chunk 数据块
        auto chunkResult = allocator.allocate(poolTotalSize, 8U);
        if (!chunkResult.has_value())
        {
            ZEROCP_LOG(Error, "Failed to allocate chunk memory for Pool " << i);
            return false;
        }
        void* chunkMemory = chunkResult.value();
        
        // 关键：计算池首偏移量（相对于数据区基地址）
        // 用于多进程通信：其他进程可以通过 dataOffset 计算出该池在其地址空间中的位置
        // 计算公式：具体chunk地址 = 数据区基地址 + dataOffset + chunk索引 * actualChunkSize
        uint64_t dataOffset = static_cast<char*>(chunkMemory) - static_cast<char*>(baseAddress);
        
        // 关键：设置 MemPool 的 rawMemory 指针和 dataOffset
        // 这个地址指向这个 Pool 管理的内存块的起始位置
        pool.setRawMemory(chunkMemory, dataOffset);
    }
    
    ZEROCP_LOG(Info, "ChunkMemoryLayout completed successfully");
    return true;
}


} // namespace Memory
} // namespace ZeroCP
