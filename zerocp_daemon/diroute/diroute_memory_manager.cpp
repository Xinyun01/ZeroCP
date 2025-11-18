#include "diroute_memory_manager.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <iostream>

namespace ZeroCP
{
namespace Diroute
{

// ============================================================================
// Static factory method - iceoryx pattern for memory pool creation
// ============================================================================

std::expected<DirouteMemoryManager, MemoryManagerError>
DirouteMemoryManager::createMemoryPool(const Config& config) noexcept
{
    ZEROCP_LOG(Info, "=== DirouteMemoryManager: Starting memory pool creation ===");
    
    // -------------------------------------------------------------------------
    // Phase 1: Create POSIX shared memory
    // -------------------------------------------------------------------------
    ZEROCP_LOG(Info, "[Phase 1] Creating shared memory: " << config.shmName);
    ZEROCP_LOG(Info, "[Phase 1] Size: " << config.shmSize << " bytes");
    
    auto shmResult = createSharedMemory(config);
    if (!shmResult)
    {
        ZEROCP_LOG(Error, "[Phase 1] Failed to create shared memory");
        return std::unexpected(shmResult.error());
    }
    
    auto shm = std::move(*shmResult);
    void* baseAddress = shm.getBaseAddress();
    
    ZEROCP_LOG(Info, "[Phase 1] Shared memory created at address: " << baseAddress);
    ZEROCP_LOG(Info, "[Phase 1] Ownership: " << (shm.hasOwnership() ? "YES" : "NO"));

    // -------------------------------------------------------------------------
    // Phase 2: Construct DirouteComponents using placement new
    // -------------------------------------------------------------------------
    ZEROCP_LOG(Info, "[Phase 2] Constructing DirouteComponents in shared memory");
    
    auto componentsResult = constructComponents(baseAddress);
    if (!componentsResult)
    {
        ZEROCP_LOG(Error, "[Phase 2] Failed to construct DirouteComponents");
        return std::unexpected(componentsResult.error());
    }
    
    auto* components = *componentsResult;
    ZEROCP_LOG(Info, "[Phase 2] DirouteComponents constructed at: " << components);

    // -------------------------------------------------------------------------
    // Phase 3: Distributed construction - HeartbeatBlock
    // -------------------------------------------------------------------------
    ZEROCP_LOG(Info, "[Phase 3] Constructing HeartbeatBlock using placement new");
    
    auto heartbeatResult = constructHeartbeatBlock(components);
    if (!heartbeatResult)
    {
        ZEROCP_LOG(Error, "[Phase 3] Failed to construct HeartbeatBlock");
        // Cleanup: destroy DirouteComponents
        components->~DirouteComponents();
        return std::unexpected(heartbeatResult.error());
    }
    
    ZEROCP_LOG(Info, "[Phase 3] HeartbeatBlock constructed successfully");
    ZEROCP_LOG(Info, "=== Memory pool creation completed ===");

    // -------------------------------------------------------------------------
    // Return initialized memory manager
    // -------------------------------------------------------------------------
    return DirouteMemoryManager(std::move(shm), components);
}

// ============================================================================
// Phase 1: Create shared memory
// ============================================================================

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

// ============================================================================
// Phase 2: Construct DirouteComponents in shared memory
// ============================================================================

std::expected<DirouteComponents*, MemoryManagerError>
DirouteMemoryManager::constructComponents(void* baseAddress) noexcept
{
    if (baseAddress == nullptr)
    {
        return std::unexpected(MemoryManagerError::INVALID_BASE_ADDRESS);
    }
    
    try
    {
        // Use placement new to construct DirouteComponents in shared memory
        // At this point, only the struct is constructed, internal components
        // (HeartbeatBlock) are just reserved memory (aligned_storage)
        DirouteComponents* components = new (baseAddress) DirouteComponents();
        return components;
    }
    catch (...)
    {
        return std::unexpected(MemoryManagerError::COMPONENT_CONSTRUCTION_FAILED);
    }
}

// ============================================================================
// Phase 3: Construct HeartbeatBlock
// ============================================================================

std::expected<void, MemoryManagerError>
DirouteMemoryManager::constructHeartbeatBlock(DirouteComponents* components) noexcept
{
    if (components == nullptr)
    {
        return std::unexpected(MemoryManagerError::COMPONENT_CONSTRUCTION_FAILED);
    }
    
    try
    {
        // Use the distributed construction method to construct HeartbeatBlock
        // This uses placement new internally to construct in the aligned_storage
        components->constructHeartbeatBlock();
        return {};
    }
    catch (...)
    {
        return std::unexpected(MemoryManagerError::HEARTBEAT_BLOCK_CONSTRUCTION_FAILED);
    }
}

// ============================================================================
// Constructor and destructor
// ============================================================================

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
        ZEROCP_LOG(Info, "DirouteMemoryManager: Destroying components");
        
        // Explicitly call destructor on DirouteComponents
        // This will destroy HeartbeatBlock in reverse order (LIFO)
        m_components->~DirouteComponents();
        m_components = nullptr;
        m_initialized = false;
        
        ZEROCP_LOG(Info, "DirouteMemoryManager: Components destroyed");
    }
    // m_sharedMemory destructor will unmap the shared memory
}

// ============================================================================
// Accessors
// ============================================================================

DirouteComponents* DirouteMemoryManager::getComponents() noexcept
{
    return m_components;
}

const DirouteComponents* DirouteMemoryManager::getComponents() const noexcept
{
    return m_components;
}

zerocp::memory::HeartbeatBlock& DirouteMemoryManager::getHeartbeatBlock() noexcept
{
    return m_components->heartbeatBlock();
}

bool DirouteMemoryManager::isInitialized() const noexcept
{
    return m_initialized;
}

} // namespace Diroute
} // namespace ZeroCP
