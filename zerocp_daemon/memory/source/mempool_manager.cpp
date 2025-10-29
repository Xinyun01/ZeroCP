#include "memorymanager.hpp"
#include "mempool_config.hpp"
#include "mempool.hpp"
#include "chunk_manager.hpp"
#include "mpmclockfreelist.hpp"
#include <iostream>

namespace ZeroCP
{

namespace Memory
{

// ==================== 静态成员变量定义 ====================

MemPoolManager* MemPoolManager::s_instance = nullptr;
std::mutex MemPoolManager::s_mutex;

// ==================== 单例模式实现 ====================

MemPoolManager& MemPoolManager::getInstance(const MemPoolConfig& config) noexcept
{
    if (s_instance == nullptr)
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        // Double-checked locking
        if (s_instance == nullptr)
        {
            s_instance = new MemPoolManager(config);
        }
    }
    return *s_instance;
}

MemPoolManager* MemPoolManager::getInstanceIfInitialized() noexcept
{
    return s_instance;
}

// ==================== 构造函数 ====================

MemPoolManager::MemPoolManager(const MemPoolConfig& config) noexcept
    : m_config(config)
{
    // 引用保证非空，无需验证
}

uint64_t MemPoolManager::getChunkMemorySize() const noexcept
{
    uint64_t totalMemorySize{0};
    uint64_t chunkNums{0};
    //chunk池+chunkment
    for(const auto& entry : m_config.m_memPoolEntries)
    {
        //记录静态链表所需要的内存大小
        auto poolmemorySize = align(Concurrent::MPMC_LockFree_List::requiredIndexMemorySize(entry.m_poolCount), 8U);
        totalMemorySize += poolmemorySize;
        chunkNums += entry.m_poolCount;
    }
    //每一个chunk都需要一个chunkManager 对象进行管理，
    totalMemorySize += chunkNums * align(sizeof(ChunkManager), 8U);
    //整个chunkmanger 需要一个静态数组链表去管理chunkManager 对象
    totalMemorySize += align(Concurrent::MPMC_LockFree_List::requiredIndexMemorySize(chunkNums), 8U);
    return totalMemorySize;
}

uint64_t MemPoolManager::getManagementMemorySize() const noexcept
{
    uint64_t totalMemorySize{0};
    uint64_t chunkNums{0};
    for(const auto& entry : m_config.m_memPoolEntries)
    {
        totalMemorySize += entry.m_poolSize * entry.m_poolCount;
        chunkNums += entry.m_poolCount;
    }
    return totalMemorySize;
}

uint64_t MemPoolManager::getTotalMemorySize() const noexcept
{
    return getChunkMemorySize() + getManagementMemorySize();
}

// ==================== 生命周期管理 ====================

bool MemPoolManager::initialize() noexcept
{
    // 创建管理内存和chunk内存
    auto ChunkMemorySize = getChunkMemorySize();
    auto ManagementMemorySize = getManagementMemorySize();
    void* managementAddress = MemPoolManager::createSharedMemory(
                                                                    "/zerocp_memory_management",
                                                                    AccessMode::READ_WRITE,
                                                                    OpenMode::OPEN_OR_CREATE,
                                                                    Perms(0600),
                                                                    ManagementMemorySize
                                                                );
    if (managementMemory == nullptr)
    {
        ZEROCP_LOG(Error, "Failed to create shared memory for management");
        return false;
    }
    // 创建chunk内存
    void* chunkMemoryAddress = MemPoolManager::createSharedMemory(
                                                                    "/zerocp_memory_chunk",
                                                                    AccessMode::READ_WRITE,
                                                                    OpenMode::OPEN_OR_CREATE,
                                                                    Perms(0600),
                                                                    ChunkMemorySize
                                                                );
    if (chunkMemoryAddress == nullptr)
    {
        ZEROCP_LOG(Error, "Failed to create shared memory for chunk");
        return false;
    }   
    auto resultPool = MemPoolAllocator::layoutMemory(managementAddress, ManagementMemorySize);
    auto resultChunk = MemPoolAllocator::layoutMemory(chunkMemoryAddress, ChunkMemorySize);
    if (!managementAddress || !chunkMemoryAddress)
    {
        ZEROCP_LOG(Error, "Failed to layout memory");
        return false;
    }
    return true;
}

bool MemPoolManager::isInitialized() const noexcept
{
    return m_initialized;
}

// ==================== 核心分配/释放接口 ====================

ChunkManager* MemPoolManager::getChunk(uint64_t size) noexcept
{
    if (!m_initialized)
    {
        std::cerr << "[MemPoolManager] Not initialized" << std::endl;
        return nullptr;
    }
    
    // 选择合适的内存池（选择最小满足条件的池）
    int32_t poolIndex = -1;
    uint64_t minSize = UINT64_MAX;
    
    for (size_t i = 0; i < m_config.m_memPoolEntries.size(); ++i)
    {
        const auto& entry = m_config.m_memPoolEntries[i];
        if (entry.m_poolSize >= size && entry.m_poolSize < minSize)
        {
            minSize = entry.m_poolSize;
            poolIndex = static_cast<int32_t>(i);
        }
    }
    
    if (poolIndex < 0)
    {
        std::cerr << "[MemPoolManager] No suitable pool for size: " << size << std::endl;
        return nullptr;
    }
    
    // 从内存池获取 chunk
    MemPool& pool = m_mempools[static_cast<size_t>(poolIndex)];
    void* chunkData = pool.getChunk();
    if (chunkData == nullptr)
    {
        std::cerr << "[MemPoolManager] Pool " << poolIndex << " exhausted" << std::endl;
        return nullptr;
    }
    
    // 从 ChunkManager 对象池获取管理器对象
    ChunkManager* manager = reinterpret_cast<ChunkManager*>(m_chunkManagerPool[0].getChunk());
    if (manager == nullptr)
    {
        std::cerr << "[MemPoolManager] ChunkManager pool exhausted" << std::endl;
        // 归还 chunk 到原池
        pool.freeChunk(chunkData);
        return nullptr;
    }
    
    // 初始化 ChunkManager
    manager->m_chunkHeader = chunkData;
    manager->m_mempool = &pool;
    manager->m_chunkManagementPool = &m_chunkManagerPool[0];
    manager->m_refCount.store(1, std::memory_order_release);
    
    return manager;
}

bool MemPoolManager::releaseChunk(ChunkManager* chunkManager) noexcept
{
    if (chunkManager == nullptr)
    {
        std::cerr << "[MemPoolManager] Null ChunkManager pointer" << std::endl;
        return false;
    }
    
    // 减少引用计数
    uint64_t prevCount = chunkManager->m_refCount.fetch_sub(1, std::memory_order_acq_rel);
    
    if (prevCount == 1)
    {
        // 引用计数归零，真正释放
        MemPool* dataPool = chunkManager->m_mempool.get();
        MemPool* mgmtPool = chunkManager->m_chunkManagementPool.get();
        void* chunkData = chunkManager->m_chunkHeader.get();
        
        if (dataPool && chunkData)
        {
            dataPool->freeChunk(chunkData);
        }
        
        if (mgmtPool)
        {
            mgmtPool->freeChunk(chunkManager);
        }
        
        return true;
    }
    else if (prevCount > 1)
    {
        // 还有其他引用，仅减少计数
        return true;
    }
    else
    {
        std::cerr << "[MemPoolManager] Double free detected!" << std::endl;
        return false;
    }
}

void MemPoolManager::printAllPoolStats() const noexcept
{
    std::cout << "==================== MemPoolManager Stats ====================" << std::endl;
    std::cout << "Initialized: " << (m_initialized ? "Yes" : "No") << std::endl;
    std::cout << "Total Memory Size: " << getTotalMemorySize() << " bytes" << std::endl;
    std::cout << "Data Pools: " << m_mempools.size() << std::endl;
    
    for (size_t i = 0; i < m_mempools.size(); ++i)
    {
        const MemPool& pool = m_mempools[i];
        std::cout << "  Pool[" << i << "]: "
                  << "ChunkSize=" << pool.getChunkSize() << " bytes, "
                  << "Total=" << pool.getTotalChunks() << ", "
                  << "Used=" << pool.getUsedChunks() << ", "
                  << "Free=" << pool.getFreeChunks() << std::endl;
    }
    
    if (!m_chunkManagerPool.empty())
    {
        const MemPool& mgmtPool = m_chunkManagerPool[0];
        std::cout << "ChunkManager Pool: "
                  << "Total=" << mgmtPool.getTotalChunks() << ", "
                  << "Used=" << mgmtPool.getUsedChunks() << ", "
                  << "Free=" << mgmtPool.getFreeChunks() << std::endl;
    }
    
    std::cout << "===============================================================" << std::endl;
}

vector<MemPool, 16>& MemPoolManager::getMemPools() noexcept
{
    return m_mempools;
}

vector<MemPool, 1>& MemPoolManager::getChunkManagerPool() noexcept
{
    return m_chunkManagerPool;
}

} // namespace Memory

} // namespace ZeroCP