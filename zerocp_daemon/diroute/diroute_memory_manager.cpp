#include "diroute_memory_manager.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <iostream>

namespace ZeroCP
{
namespace Diroute
{

std::expected<DirouteMemoryManager, MemoryManagerError>
DirouteMemoryManager::createMemoryPool(const Config& config) noexcept
{
    ZEROCP_LOG(Info, "Creating memory pool: " << config.shmName << " (" << config.shmSize << " bytes)");
    
    auto shmResult = createSharedMemory(config);
    if (!shmResult)
    {
        ZEROCP_LOG(Error, "Failed to create shared memory");
        return std::unexpected(shmResult.error());
    }
    
    auto shm = std::move(*shmResult);
    void* baseAddress = shm.getBaseAddress();
    
    auto componentsResult = constructComponents(baseAddress);
    if (!componentsResult)
    {
        ZEROCP_LOG(Error, "Failed to construct DirouteComponents");
        return std::unexpected(componentsResult.error());
    }
    
    auto* components = *componentsResult;
    components->setBaseAddress(baseAddress);
    
    auto heartbeatResult = constructHeartbeatPool(components);
    if (!heartbeatResult)
    {
        ZEROCP_LOG(Error, "Failed to construct HeartbeatPool");
        components->~DirouteComponents();
        return std::unexpected(heartbeatResult.error());
    }

    auto queueResult = constructReceiveQueues(components);
    if (!queueResult)
    {
        ZEROCP_LOG(Error, "Failed to construct receive queues");
        components->~DirouteComponents();
        return std::unexpected(queueResult.error());
    }

    auto descriptorResult = initializeQueueDescriptors(components);
    if (!descriptorResult)
    {
        ZEROCP_LOG(Error, "Failed to initialize queue descriptors");
        components->~DirouteComponents();
        return std::unexpected(descriptorResult.error());
    }
    
    ZEROCP_LOG(Info, "Memory pool created successfully at " << baseAddress);

    return DirouteMemoryManager(std::move(shm), components);
}

std::expected<ZeroCP::Details::PosixSharedMemoryObject, MemoryManagerError>
DirouteMemoryManager::createSharedMemory(const Config& config) noexcept
{
    auto shmResult = ZeroCP::Details::PosixSharedMemoryObjectBuilder()
        .name(config.shmName)
        .memorySize(config.shmSize)
        .accessMode(config.accessMode)
        .openMode(config.openMode)
        .permissions(config.permissions)
        .create();
    
    if (!shmResult)
    {
        return std::unexpected(MemoryManagerError::SHARED_MEMORY_CREATION_FAILED);
    }
    
    return std::move(*shmResult);
}

std::expected<DirouteComponents*, MemoryManagerError>
DirouteMemoryManager::constructComponents(void* baseAddress) noexcept
{
    if (baseAddress == nullptr)
    {
        return std::unexpected(MemoryManagerError::INVALID_BASE_ADDRESS);
    }
    
    try
    {
        // placement new: 在共享内存中构造 DirouteComponents
        // 此时内部组件（HeartbeatPool）仅预留内存，尚未构造
        DirouteComponents* components = new (baseAddress) DirouteComponents();
        return components;
    }
    catch (...)
    {
        return std::unexpected(MemoryManagerError::COMPONENT_CONSTRUCTION_FAILED);
    }
}

std::expected<void, MemoryManagerError>
DirouteMemoryManager::constructHeartbeatPool(DirouteComponents* components) noexcept
{
    if (components == nullptr)
    {
        return std::unexpected(MemoryManagerError::COMPONENT_CONSTRUCTION_FAILED);
    }
    
    try
    {
        // 分布式构造：内部使用 placement new 在预留内存中构造
        components->constructHeartbeatPool();
        return {};
    }
    catch (...)
    {
        return std::unexpected(MemoryManagerError::HEARTBEAT_BLOCK_CONSTRUCTION_FAILED);
    }
}

std::expected<void, MemoryManagerError>
DirouteMemoryManager::constructReceiveQueues(DirouteComponents* components) noexcept
{
    if (components == nullptr)
    {
        return std::unexpected(MemoryManagerError::COMPONENT_CONSTRUCTION_FAILED);
    }

    try
    {
        components->constructReceiveQueues();
        return {};
    }
    catch (...)
    {
        return std::unexpected(MemoryManagerError::RECEIVE_QUEUE_CONSTRUCTION_FAILED);
    }
}

std::expected<void, MemoryManagerError>
DirouteMemoryManager::initializeQueueDescriptors(DirouteComponents* components) noexcept
{
    if (components == nullptr)
    {
        return std::unexpected(MemoryManagerError::COMPONENT_CONSTRUCTION_FAILED);
    }

    try
    {
        components->initializeQueueDescriptors();
        return {};
    }
    catch (...)
    {
        return std::unexpected(MemoryManagerError::QUEUE_DESCRIPTOR_INITIALIZATION_FAILED);
    }
}

DirouteMemoryManager::DirouteMemoryManager(ZeroCP::Details::PosixSharedMemoryObject&& shm,
                                           DirouteComponents* components) noexcept
    : m_sharedMemory(std::move(shm))
    , m_components(components)
    , m_initialized(true)
{
}

DirouteMemoryManager::~DirouteMemoryManager() noexcept
{
    if (m_initialized && m_components != nullptr)
    {
        // 显式析构：按 LIFO 顺序销毁 HeartbeatPool
        m_components->~DirouteComponents();
        m_components = nullptr;
        m_initialized = false;
    }
}

DirouteComponents* DirouteMemoryManager::getComponents() noexcept
{
    return m_components;
}

const DirouteComponents* DirouteMemoryManager::getComponents() const noexcept
{
    return m_components;
}

zerocp::memory::HeartbeatPool& DirouteMemoryManager::getHeartbeatPool() noexcept
{
    return m_components->heartbeatPool();
}

bool DirouteMemoryManager::isInitialized() const noexcept
{
    return m_initialized;
}

} // namespace Diroute
} // namespace ZeroCP
