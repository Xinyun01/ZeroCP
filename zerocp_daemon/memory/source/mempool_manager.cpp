#include "mempool_manager.hpp"
#include "mempool_config.hpp"
#include "mempool.hpp"
#include "chunk_manager.hpp"
#include "chunk_header.hpp"
#include "mpmclockfreelist.hpp"
#include "memory.hpp"
#include "mempool_allocator.hpp"
#include "posixshm_provider.hpp"
#include <iostream>
#include <cstring>  // memset
#include <fcntl.h>  // O_CREAT, O_EXCL
#include "logging.hpp"

using ZeroCP::Memory::align;
using ZeroCP::Memory::PosixShmProvider;
using ZeroCP::Memory::Name_t;
using ZeroCP::AccessMode;
using ZeroCP::OpenMode;
using ZeroCP::Perms;
namespace ZeroCP
{

namespace Memory
{
// ==================== 静态成员变量定义 ====================

MemPoolManager* MemPoolManager::s_instance = nullptr;
void* MemPoolManager::s_managementBaseAddress = nullptr;
void* MemPoolManager::s_chunkBaseAddress = nullptr;
size_t MemPoolManager::s_managementMemorySize = 0;
size_t MemPoolManager::s_chunkMemorySize = 0;
sem_t* MemPoolManager::s_initSemaphore = SEM_FAILED;
const char* MemPoolManager::MGMT_SHM_NAME = "zerocp_memory_management";
const char* MemPoolManager::CHUNK_SHM_NAME = "zerocp_memory_chunk";
const char* MemPoolManager::SEM_NAME = "/zerocp_init_sem";
std::unique_ptr<PosixShmProvider> MemPoolManager::s_mgmtProvider = nullptr;
std::unique_ptr<PosixShmProvider> MemPoolManager::s_chunkProvider = nullptr;
bool MemPoolManager::s_isOwner = false;

// ==================== 单例模式实现（共享内存版本） ====================

bool MemPoolManager::createSharedInstance(const MemPoolConfig& config) noexcept
{
    // 1. 创建临时 MemPoolManager 来计算内存大小
    MemPoolConfig tempConfig = config;  // 拷贝配置
    MemPoolManager tempMgr(tempConfig);
    
    // 管理区大小 = MemPoolManager对象(包含vectors) + 管理数据结构(freeLists + ChunkManagers)
    // 注意：MemPoolManager 对象已经包含了 m_memPoolVector 和 m_chunkManagementPool 两个成员
    // 所以不需要单独为 vectors 分配空间
    size_t managerObjSize = align(sizeof(MemPoolManager), 8U);
    uint64_t managementDataSize = tempMgr.getManagementMemorySize();
    uint64_t managementSize = managerObjSize + managementDataSize;
    uint64_t chunkSize = align(tempMgr.getChunkMemorySize(), 8U);
    
    ZEROCP_LOG(Info, "Memory layout calculation:");
    ZEROCP_LOG(Info, "  - MemPoolManager object (includes vectors): " << managerObjSize << " bytes");
    ZEROCP_LOG(Info, "  - Management data (freeLists + ChunkManagers): " << managementDataSize << " bytes");
    ZEROCP_LOG(Info, "  - Total management memory: " << managementSize << " bytes");
    ZEROCP_LOG(Info, "  - Chunk memory: " << chunkSize << " bytes");
    ZEROCP_LOG(Info, "  - Total memory needed: " << (managementSize + chunkSize) << " bytes");
    
    // 2. 创建管理区共享内存
    s_mgmtProvider = std::make_unique<PosixShmProvider>(
        Name_t(MGMT_SHM_NAME),
        managementSize,
        AccessMode::ReadWrite,
        OpenMode::OpenOrCreate,
        Perms::OwnerAll
    );
    
    auto mgmtResult = s_mgmtProvider->createMemory();
    if (!mgmtResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to create management shared memory");
        s_mgmtProvider.reset();
        return false;
    }
    void* managementAddress = mgmtResult.value();
    ZEROCP_LOG(Info, "Management memory created at: " << managementAddress);
    
    // 3. 创建数据区（chunks）共享内存
    s_chunkProvider = std::make_unique<PosixShmProvider>(
        Name_t(CHUNK_SHM_NAME),
        chunkSize,
        AccessMode::ReadWrite,
        OpenMode::OpenOrCreate,
        Perms::OwnerAll
    );
    
    auto chunkResult = s_chunkProvider->createMemory();
    if (!chunkResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to create chunk shared memory");
        s_mgmtProvider.reset();
        s_chunkProvider.reset();
        return false;
    }
    void* chunkMemoryAddress = chunkResult.value();
    ZEROCP_LOG(Info, "Chunk memory created at: " << chunkMemoryAddress);
    
    // 4. 使用命名信号量保证只初始化一次
    s_initSemaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
    bool isFirstProcess = (s_initSemaphore != SEM_FAILED);
    
    ZEROCP_LOG(Info, "Semaphore check: isFirstProcess=" << isFirstProcess);
    
    if (!isFirstProcess)
    {
        // 不是第一个进程，打开已存在的信号量
        s_initSemaphore = sem_open(SEM_NAME, 0);
        if (s_initSemaphore == SEM_FAILED)
        {
            ZEROCP_LOG(Error, "Failed to open existing semaphore, errno=" << errno);
            return false;
        }
        ZEROCP_LOG(Info, "Opened existing semaphore");
    }
    
    // 5. 等待信号量（加锁）
    ZEROCP_LOG(Info, "Waiting for semaphore...");
    if (sem_wait(s_initSemaphore) != 0)
    {
        ZEROCP_LOG(Error, "Failed to wait on semaphore, errno=" << errno);
        return false;
    }
    ZEROCP_LOG(Info, "Acquired semaphore");
    
    // 6. 在共享内存中构造或获取 MemPoolManager 实例
    // 关键设计：MemPoolManager 对象本身在共享内存中（使用 placement new）
    // 每个进程只需设置 s_instance 指向共享内存中的同一个对象
    
    // 管理区内存布局：[MemPoolManager对象(含vectors)] [管理数据结构(freeLists + ChunkManagers)]
    // 按照 memory.md 的描述：
    // - 第1部分：MemPoolManager 对象本身（约 500 字节，包含 m_memPoolVector 和 m_chunkManagementPool）
    // - 第2部分：每个数据池的静态链表（freeLists）
    // - 第3部分：chunkManagementPool 的静态链表
    
    // managerAddress：管理区共享内存中 MemPoolManager 对象本身的起始地址（第1部分，含vector等）
    void* managerAddress = managementAddress;
    // actualManagementStart：管理区共享内存中 管理数据结构区（freeLists、ChunkManagers等）的起始地址（第2/3/4部分）
    void* actualManagementStart = static_cast<char*>(managementAddress) + managerObjSize;
    // actualManagementSize：管理数据结构区（不含MemPoolManager对象本身）的大小
    size_t actualManagementSize = managementDataSize;
    
    if (isFirstProcess)
    {
        ZEROCP_LOG(Info, "First process: constructing MemPoolManager in shared memory");
        
        // 使用 placement new 在共享内存中构造 MemPoolManager
        s_instance = new (managerAddress) MemPoolManager(config);
        s_isOwner = true;  // 标记为拥有者
        
        // 创建 MemPoolAllocator 实例进行内存布局
        MemPoolAllocator allocator(config, managementAddress);
        
        // 布局管理区内存（填充 vector 内容，分配 freeList 等）
        if (!allocator.ManagementMemoryLayout(actualManagementStart, actualManagementSize,
                                             s_instance->m_mempools, 
                                             s_instance->m_chunkManagerPool))
        {
            ZEROCP_LOG(Error, "Failed to layout management memory");
            s_instance->~MemPoolManager();
            s_instance = nullptr;
            sem_post(s_initSemaphore);
            return false;
        }
        
        // 布局数据区内存（分配 chunk 块并设置到 MemPool，同时记录 dataOffset）
        if (!allocator.ChunkMemoryLayout(chunkMemoryAddress, chunkSize,
                                        s_instance->m_mempools))
        {
            ZEROCP_LOG(Error, "Failed to layout chunk memory");
            s_instance->~MemPoolManager();
            s_instance = nullptr;
            sem_post(s_initSemaphore);
            return false;
        }
        
        ZEROCP_LOG(Info, "Shared memory layout initialized successfully");
    }
    else
    {
        ZEROCP_LOG(Info, "Attaching to existing shared memory");
        
        // 其他进程：直接使用 mmap 返回的地址，它指向共享内存中已存在的 MemPoolManager 对象
        s_instance = static_cast<MemPoolManager*>(managerAddress);
        s_isOwner = false;  // 不是拥有者
    }
    
    // 7. 保存共享内存基地址和大小（进程本地变量）
    s_managementBaseAddress = managementAddress;
    s_chunkBaseAddress = chunkMemoryAddress;
    s_managementMemorySize = managementSize;
    s_chunkMemorySize = chunkSize;
    
    // 8. 解锁信号量
    sem_post(s_initSemaphore);
    
    ZEROCP_LOG(Info, "MemPoolManager shared instance created successfully");
    return true;
}

bool MemPoolManager::attachToSharedInstance() noexcept
{
    // 如果已经连接，直接返回成功
    if (s_instance != nullptr)
    {
        ZEROCP_LOG(Info, "Already attached to shared instance");
        return true;
    }
    
    // 1. 打开管理区共享内存（只读模式，不创建）
    s_mgmtProvider = std::make_unique<PosixShmProvider>(
        Name_t(MGMT_SHM_NAME),
        0,  // 大小会从已存在的共享内存中获取
        AccessMode::ReadWrite,
        OpenMode::OpenExisting,  // 只打开已存在的
        Perms::OwnerAll
    );
    
    auto mgmtResult = s_mgmtProvider->createMemory();
    if (!mgmtResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to open management shared memory - server may not be running");
        s_mgmtProvider.reset();
        return false;
    }
    void* managementAddress = mgmtResult.value();
    ZEROCP_LOG(Info, "Opened management memory at: " << managementAddress);
    
    // 2. 打开数据区共享内存
    s_chunkProvider = std::make_unique<PosixShmProvider>(
        Name_t(CHUNK_SHM_NAME),
        0,
        AccessMode::ReadWrite,
        OpenMode::OpenExisting,
        Perms::OwnerAll
    );
    
    auto chunkResult = s_chunkProvider->createMemory();
    if (!chunkResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to open chunk shared memory");
        s_mgmtProvider.reset();
        s_chunkProvider.reset();
        return false;
    }
    void* chunkMemoryAddress = chunkResult.value();
    ZEROCP_LOG(Info, "Opened chunk memory at: " << chunkMemoryAddress);
    
    // 3. 打开信号量
    s_initSemaphore = sem_open(SEM_NAME, 0);
    if (s_initSemaphore == SEM_FAILED)
    {
        ZEROCP_LOG(Error, "Failed to open semaphore, errno=" << errno);
        return false;
    }
    
    // 4. 获取共享内存中的 MemPoolManager 实例
    // 客户端不需要构造，直接使用服务端创建的实例
    s_instance = static_cast<MemPoolManager*>(managementAddress);
    s_isOwner = false;  // 客户端不是拥有者
    
    // 5. 保存共享内存基地址（从 PosixShmProvider 获取实际大小）
    s_managementBaseAddress = managementAddress;
    s_chunkBaseAddress = chunkMemoryAddress;
    // 注意：这里我们不知道确切的大小，但不影响使用
    // 因为所有的内存布局信息都在共享内存的 MemPoolManager 对象中
    
    ZEROCP_LOG(Info, "Successfully attached to shared instance");
    return true;
}

MemPoolManager* MemPoolManager::getInstanceIfInitialized() noexcept
{
    return s_instance;
}

void MemPoolManager::destroySharedInstance() noexcept
{
    if (s_instance != nullptr)
    {
        ZEROCP_LOG(Info, "Destroying shared instance (isOwner=" << s_isOwner << ")");
        
        // 只有拥有者才需要调用析构函数
        // MemPoolManager 对象在共享内存中，使用 placement new 构造
        // 所以需要手动调用析构函数，但不能使用 delete
        if (s_isOwner)
        {
            s_instance->~MemPoolManager();
        }
        
        s_instance = nullptr;
        
        // 清空静态变量（进程本地）
        s_managementBaseAddress = nullptr;
        s_chunkBaseAddress = nullptr;
        s_managementMemorySize = 0;
        s_chunkMemorySize = 0;
        s_isOwner = false;
    }
    
    // 释放共享内存提供者（会自动取消映射和删除共享内存）
    s_mgmtProvider.reset();
    s_chunkProvider.reset();
    
    // 关闭信号量
    if (s_initSemaphore != SEM_FAILED)
    {
        sem_close(s_initSemaphore);
        s_initSemaphore = SEM_FAILED;
    }
    
    // 取消链接信号量（最后一个进程）
    sem_unlink(SEM_NAME);
    
    ZEROCP_LOG(Info, "Shared instance destroyed");
}

// ==================== 构造函数 ====================

MemPoolManager::MemPoolManager(const MemPoolConfig& config) noexcept
    : m_config(config)
    , m_mempools()
    , m_chunkManagerPool()
{
    // 引用保证非空，无需验证
    // vector 默认构造为空，后续通过 MemPoolAllocator 填充
}

uint64_t MemPoolManager::getChunkMemorySize() const noexcept
{
    // 数据区共享内存：存储所有 MemPool 的实际数据块
    // 每个 chunk = ChunkHeader + 用户数据
    uint64_t totalMemorySize{0};
    for(const auto& entry : m_config.m_memPoolEntries)
    {
        // 每个 chunk 的实际大小 = ChunkHeader + chunkSize（用户数据）
        uint64_t actualChunkSize = align(sizeof(ChunkHeader) + entry.m_chunkSize, 8U);
        // 每个池的总数据大小 = chunk实际大小 × chunk数量
        totalMemorySize += actualChunkSize * entry.m_chunkCount;
    }
    return totalMemorySize;
}

uint64_t MemPoolManager::getManagementMemorySize() const noexcept
{
    // 管理区共享内存：存储 freeList + ChunkManager 对象
    uint64_t totalMemorySize{0};
    uint64_t chunkNums{0};
    
    // 1. 计算所有 MemPool 的 freeList 大小
    for(const auto& entry : m_config.m_memPoolEntries)
    {
        // 每个池的 freeList（MPMC 无锁链表索引数组）
        auto poolmemorySize = align(Concurrent::MPMC_LockFree_List::requiredIndexMemorySize(entry.m_chunkCount), 8U);
        totalMemorySize += poolmemorySize;
        chunkNums += entry.m_chunkCount;
    }
    
    // 2. 所有 ChunkManager 对象的大小
    totalMemorySize += chunkNums * align(sizeof(ChunkManager), 8U);
    
    // 3. ChunkManagerPool 的 freeList 大小
    totalMemorySize += align(Concurrent::MPMC_LockFree_List::requiredIndexMemorySize(chunkNums), 8U);
    
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
    
    // 创建管理区共享内存
    PosixShmProvider mgmtProvider(
        Name_t("/zerocp_memory_management"),
        ManagementMemorySize,
        AccessMode::ReadWrite,
        OpenMode::OpenOrCreate,
        Perms::OwnerAll
    );
    
    auto mgmtResult = mgmtProvider.createMemory();
    if (!mgmtResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to create shared memory for management");
        return false;
    }
    void* managementAddress = mgmtResult.value();
    
    // 创建chunk区共享内存
    PosixShmProvider chunkProvider(
        Name_t("/zerocp_memory_chunk"),
        ChunkMemorySize,
        AccessMode::ReadWrite,
        OpenMode::OpenOrCreate,
        Perms::OwnerAll
    );
    
    auto chunkResult = chunkProvider.createMemory();
    if (!chunkResult.has_value())
    {
        ZEROCP_LOG(Error, "Failed to create shared memory for chunk");
        return false;
    }
    void* chunkMemoryAddress = chunkResult.value();
    
    // 创建MemPoolAllocator实例进行内存布局
    MemPoolAllocator allocator(m_config, chunkMemoryAddress);
    
    // 布局管理区内存
    if (!allocator.ManagementMemoryLayout(managementAddress, ManagementMemorySize, m_mempools, m_chunkManagerPool))
    {
        ZEROCP_LOG(Error, "Failed to layout management memory");
        return false;
    }
    
    // 布局chunk区内存
    if (!allocator.ChunkMemoryLayout(chunkMemoryAddress, ChunkMemorySize, m_mempools))
    {
        ZEROCP_LOG(Error, "Failed to layout chunk memory");
        return false;
    }
    
    ZEROCP_LOG(Info, "MemPoolManager initialized successfully");
    return true;
}

// ==================== 核心分配/释放接口 ====================
// TODO: 以下方法需要在 MemPool 实现 getChunk/freeChunk 后启用

ChunkManager* MemPoolManager::getChunk(uint64_t size) noexcept
{
    // TODO: 实现需要 MemPool::getChunk() 方法
    ZEROCP_LOG(Warn, "getChunk() not yet implemented");
    (void)size; // 避免未使用警告
    return nullptr;
}

bool MemPoolManager::releaseChunk(ChunkManager* chunkManager) noexcept
{
    // TODO: 实现需要 MemPool::freeChunk() 和 RelativePointer::get() 方法
    ZEROCP_LOG(Warn, "releaseChunk() not yet implemented");
    (void)chunkManager; // 避免未使用警告
    return false;
}

void MemPoolManager::printAllPoolStats() const noexcept
{
    std::cout << "==================== MemPoolManager Stats ====================" << std::endl;
    
    // 安全检查：先检查 m_config 是否有效
    try {
        std::cout << "Data Pools: " << m_mempools.size() << std::endl;
        
        if (!m_mempools.empty())
        {
            for (size_t i = 0; i < m_mempools.size(); ++i)
            {
                const MemPool& pool = m_mempools[i];
                std::cout << "  Pool[" << i << "]: "
                          << "ChunkSize=" << pool.getChunkSize() << " bytes, "
                          << "Total=" << pool.getTotalChunks() << ", "
                          << "Used=" << pool.getUsedChunks() << ", "
                          << "Free=" << pool.getFreeChunks() << std::endl;
            }
        }
        else
        {
            std::cout << "  (No data pools initialized)" << std::endl;
        }

        if (!m_chunkManagerPool.empty())
        {
            const MemPool& mgmtPool = m_chunkManagerPool[0];
            std::cout << "ChunkManager Pool: "
                      << "Total=" << mgmtPool.getTotalChunks() << ", "
                      << "Used=" << mgmtPool.getUsedChunks() << ", "
                      << "Free=" << mgmtPool.getFreeChunks() << std::endl;
        }
        else
        {
            std::cout << "ChunkManager Pool: (Not initialized)" << std::endl;
        }
    }
    catch (...)
    {
        std::cout << "ERROR: Exception occurred while printing stats" << std::endl;
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