#include "posixshm_provider.hpp"
#include "zerocp_foundationLib/posix/memory/include/relative_pointer.hpp"
#include <atomic>

namespace ZeroCP
{
namespace Daemon
{

// 生成唯一的段ID
static std::atomic<uint64_t> g_nextSegmentId{1};

PosixShmProvider::PosixShmProvider(const Name_t& name, const uint64_t memorySize, const AccessMode accessMode, const OpenMode openMode, const Perms permissions) noexcept
    : m_name(name), m_memorySize(memorySize), m_accessMode(accessMode), m_openMode(openMode), m_permissions(permissions)
    , m_segmentId(g_nextSegmentId.fetch_add(1, std::memory_order_relaxed))
{
}

PosixShmProvider::~PosixShmProvider() 
{
    destroyMemory();
}

std::expected<void*, PosixSharedMemoryObjectError> PosixShmProvider::createMemory() noexcept
{
    // 直接创建共享内存对象
    auto result = PosixSharedMemoryObjectBuilder()
        .setName(m_name)
        .setMemorySize(m_memorySize)
        .setAccessMode(m_accessMode)
        .setOpenMode(m_openMode)
        .setPermissions(m_permissions)
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
    
    // 注册到全局段注册表，用于 RelativePointer
    SegmentRegistry::instance().registerSegment(m_segmentId, m_baseAddress);
    
    ZEROCP_LOG(Info, "Shared memory created - Segment ID: {}, Base Address: {}", m_segmentId, m_baseAddress);
    
    return m_baseAddress;
}

std::expected<void, PosixSharedMemoryObjectError> PosixShmProvider::destroyMemory() noexcept
{
    if(m_sharedMemoryObject.has_value())
    {
        // 取消注册段
        SegmentRegistry::instance().unregisterSegment(m_segmentId);
        ZEROCP_LOG(Info, "Unregistered segment ID: {}", m_segmentId);
        
        // 重置共享内存对象
        m_sharedMemoryObject.reset();
        m_baseAddress = nullptr;
    }
    
    return {};
}

uint64_t PosixShmProvider::getSegmentId() const noexcept
{
    return m_segmentId;
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
    ZEROCP_LOG(Info, "Memory available announced for segment ID: {}", m_segmentId);
}

} // namespace Daemon
} // namespace ZeroCP

