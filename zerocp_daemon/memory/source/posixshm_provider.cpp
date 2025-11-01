#include "posixshm_provider.hpp"
#include "relative_pointer.hpp"
#include <atomic>

namespace ZeroCP
{
namespace Memory
{

// 生成唯一的池ID
static std::atomic<uint64_t> g_nextPoolId{1};

PosixShmProvider::PosixShmProvider(const Name_t& name, const uint64_t memorySize, const AccessMode accessMode, const OpenMode openMode, const Perms permissions) noexcept
    : m_name(name), m_memorySize(memorySize), m_accessMode(accessMode), m_openMode(openMode), m_permissions(permissions)
    , m_poolId(g_nextPoolId.fetch_add(1, std::memory_order_relaxed))
{
}

PosixShmProvider::~PosixShmProvider() 
{
    destroyMemory();
}

std::expected<void*, PosixSharedMemoryObjectError> PosixShmProvider::createMemory() noexcept
{
    // 直接创建共享内存对象
    auto result = Details::PosixSharedMemoryObjectBuilder()
        .name(m_name)
        .memorySize(m_memorySize)
        .accessMode(m_accessMode)
        .openMode(m_openMode)
        .permissions(m_permissions)
        .create();
   
    if(!result.has_value())
    {
        ZEROCP_LOG(Error, "Failed to create shared memory object");
        return std::unexpected(result.error());
    }
    
    // 移动共享内存对象
    m_sharedMemoryObject = std::move(result.value());
    
    // 获取基地址
    m_baseAddress = m_sharedMemoryObject->getBaseAddress();
    if(!m_baseAddress)
    {
        ZEROCP_LOG(Error, "Failed to get base address");
        return std::unexpected(PosixSharedMemoryObjectError::UNKNOWN_ERROR);
    }
    
    // 注册到全局池注册表，用于 RelativePointer
    // TODO: 需要实现 PoolRegistry 类
    // PoolRegistry::instance().registerPool(m_poolId, m_baseAddress);
    
    ZEROCP_LOG(Info, "Shared memory created - Pool ID: " << m_poolId << ", Base Address: " << m_baseAddress);
    
    return m_baseAddress;
}

std::expected<void, PosixSharedMemoryObjectError> PosixShmProvider::destroyMemory() noexcept
{
    if(m_sharedMemoryObject.has_value())
    {
        // 取消注册池
        // TODO: 需要实现 PoolRegistry 类
        // PoolRegistry::instance().unregisterPool(m_poolId);
        ZEROCP_LOG(Info, "Unregistered pool ID: " << m_poolId);
        
        // 重置共享内存对象
        m_sharedMemoryObject.reset();
        m_baseAddress = nullptr;
    }
    
    return {};
}

uint64_t PosixShmProvider::getPoolId() const noexcept
{
    return m_poolId;
}

bool PosixShmProvider::isMemoryAvailable() const noexcept
{
    return m_memoryAvailableAnnounced;
}

void* PosixShmProvider::getBaseAddress() const noexcept
{
    return m_baseAddress;
}

void PosixShmProvider::announceMemoryAvailable() noexcept
{
    m_memoryAvailableAnnounced = true;
    ZEROCP_LOG(Info, "Memory available announced for pool ID: " << m_poolId);
}

} // namespace Memory
} // namespace ZeroCP

