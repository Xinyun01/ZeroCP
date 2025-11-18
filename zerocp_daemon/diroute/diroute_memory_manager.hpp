#ifndef ZEROCP_DIROUTE_MEMORY_MANAGER_HPP
#define ZEROCP_DIROUTE_MEMORY_MANAGER_HPP

#include <expected>
#include <memory>
#include <string>

#include "diroute_components.hpp"
#include "zerocp_foundationLib/posix/memory/include/posix_sharedmemory_object.hpp"

namespace ZeroCP
{
namespace Diroute
{

/// @brief Error codes for memory manager initialization
enum class MemoryManagerError
{
    SHARED_MEMORY_CREATION_FAILED,
    COMPONENT_CONSTRUCTION_FAILED,
    HEARTBEAT_BLOCK_CONSTRUCTION_FAILED,
    INVALID_BASE_ADDRESS
};

/// @brief Memory manager for DirouteComponents following iceoryx pattern
/// This class manages the lifecycle of shared memory and distributed construction
/// of components, similar to iceoryx::roudi::MemoryManager
class DirouteMemoryManager
{
public:
    /// @brief Shared memory configuration
    struct Config
    {
        std::string shmName{"/zerocp_diroute_components"};
        uint64_t shmSize{sizeof(DirouteComponents)};
        ZeroCP::AccessMode accessMode{ZeroCP::AccessMode::ReadWrite};
        ZeroCP::OpenMode openMode{ZeroCP::OpenMode::PurgeAndCreate};
        ZeroCP::Perms permissions{ZeroCP::Perms::OwnerAll | ZeroCP::Perms::GroupRead | ZeroCP::Perms::GroupWrite};
    };

    DirouteMemoryManager() = default;
    ~DirouteMemoryManager() noexcept;

    // Delete copy and move
    DirouteMemoryManager(const DirouteMemoryManager&) = delete;
    DirouteMemoryManager& operator=(const DirouteMemoryManager&) = delete;
    DirouteMemoryManager(DirouteMemoryManager&&) = delete;
    DirouteMemoryManager& operator=(DirouteMemoryManager&&) = delete;

    /// @brief Static method to create and initialize shared memory pool (iceoryx pattern)
    /// Similar to iceoryx::roudi::MemoryManager::createAndAnnounceMemory()
    /// @param config Configuration for shared memory
    /// @return Expected containing DirouteMemoryManager or error
    [[nodiscard]] static std::expected<DirouteMemoryManager, MemoryManagerError>
    createMemoryPool(const Config& config = Config{}) noexcept;

    /// @brief Get pointer to DirouteComponents in shared memory
    [[nodiscard]] DirouteComponents* getComponents() noexcept;
    
    /// @brief Get const pointer to DirouteComponents in shared memory
    [[nodiscard]] const DirouteComponents* getComponents() const noexcept;

    /// @brief Get HeartbeatPool reference
    [[nodiscard]] zerocp::memory::HeartbeatPool& getHeartbeatPool() noexcept;

    /// @brief Check if memory pool is initialized
    [[nodiscard]] bool isInitialized() const noexcept;

private:
    /// @brief Private constructor called by createMemoryPool
    explicit DirouteMemoryManager(ZeroCP::Details::PosixSharedMemoryObject&& shm,
                                  DirouteComponents* components) noexcept;

    /// @brief Phase 1: Create POSIX shared memory
    [[nodiscard]] static std::expected<ZeroCP::Details::PosixSharedMemoryObject, MemoryManagerError>
    createSharedMemory(const Config& config) noexcept;

    /// @brief Phase 2: Construct DirouteComponents in shared memory using placement new
    [[nodiscard]] static std::expected<DirouteComponents*, MemoryManagerError>
    constructComponents(void* baseAddress) noexcept;

    /// @brief Phase 3: Distributed construction - construct HeartbeatPool
    [[nodiscard]] static std::expected<void, MemoryManagerError>
    constructHeartbeatPool(DirouteComponents* components) noexcept;

    /// @brief Shared memory object
    ZeroCP::Details::PosixSharedMemoryObject m_sharedMemory;
    
    /// @brief Pointer to DirouteComponents in shared memory
    DirouteComponents* m_components{nullptr};
    
    /// @brief Initialization flag
    bool m_initialized{false};
};

} // namespace Diroute
} // namespace ZeroCP

#endif // ZEROCP_DIROUTE_MEMORY_MANAGER_HPP
