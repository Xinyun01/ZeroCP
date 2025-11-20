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

enum class MemoryManagerError
{
    SHARED_MEMORY_CREATION_FAILED,
    COMPONENT_CONSTRUCTION_FAILED,
    HEARTBEAT_BLOCK_CONSTRUCTION_FAILED,
    RECEIVE_QUEUE_CONSTRUCTION_FAILED,
    QUEUE_DESCRIPTOR_INITIALIZATION_FAILED,
    INVALID_BASE_ADDRESS
};

/// DirouteComponents 内存管理器（iceoryx 分布式构造模式）
/// 管理共享内存生命周期和组件的分步构造
class DirouteMemoryManager
{
public:
    struct Config
    {
        std::string shmName{"zerocp_diroute_components"};
        uint64_t shmSize{sizeof(DirouteComponents)};
        ZeroCP::AccessMode accessMode{ZeroCP::AccessMode::ReadWrite};
        ZeroCP::OpenMode openMode{ZeroCP::OpenMode::PurgeAndCreate};
        ZeroCP::Perms permissions{ZeroCP::Perms::OwnerAll | ZeroCP::Perms::GroupRead | ZeroCP::Perms::GroupWrite};
    };

    DirouteMemoryManager() = default;
    ~DirouteMemoryManager() noexcept;

    DirouteMemoryManager(const DirouteMemoryManager&) = delete;
    DirouteMemoryManager& operator=(const DirouteMemoryManager&) = delete;
    DirouteMemoryManager(DirouteMemoryManager&&) noexcept = default;
    DirouteMemoryManager& operator=(DirouteMemoryManager&&) = delete;

    // 创建并初始化共享内存池（类似 iceoryx::roudi::MemoryManager::createAndAnnounceMemory）
    [[nodiscard]] static std::expected<DirouteMemoryManager, MemoryManagerError>
    createMemoryPool() noexcept
    {
        return createMemoryPool(Config{});
    }
    
    [[nodiscard]] static std::expected<DirouteMemoryManager, MemoryManagerError>
    createMemoryPool(const Config& config) noexcept;

    [[nodiscard]] DirouteComponents* getComponents() noexcept;
    [[nodiscard]] const DirouteComponents* getComponents() const noexcept;
    [[nodiscard]] zerocp::memory::HeartbeatPool& getHeartbeatPool() noexcept;
    [[nodiscard]] bool isInitialized() const noexcept;

private:
    explicit DirouteMemoryManager(ZeroCP::Details::PosixSharedMemoryObject&& shm,
                                  DirouteComponents* components) noexcept;

    // Phase 1: 创建 POSIX 共享内存
    [[nodiscard]] static std::expected<ZeroCP::Details::PosixSharedMemoryObject, MemoryManagerError>
    createSharedMemory(const Config& config) noexcept;

    // Phase 2: 使用 placement new 构造 DirouteComponents
    [[nodiscard]] static std::expected<DirouteComponents*, MemoryManagerError>
    constructComponents(void* baseAddress) noexcept;

    // Phase 3: 分布式构造 HeartbeatPool
    [[nodiscard]] static std::expected<void, MemoryManagerError>
    constructHeartbeatPool(DirouteComponents* components) noexcept;

    // Phase 4: 构造接收队列池
    [[nodiscard]] static std::expected<void, MemoryManagerError>
    constructReceiveQueues(DirouteComponents* components) noexcept;

    // Phase 5: 初始化队列描述符
    [[nodiscard]] static std::expected<void, MemoryManagerError>
    initializeQueueDescriptors(DirouteComponents* components) noexcept;

    ZeroCP::Details::PosixSharedMemoryObject m_sharedMemory;
    DirouteComponents* m_components{nullptr};
    bool m_initialized{false};
};

} // namespace Diroute
} // namespace ZeroCP

#endif // ZEROCP_DIROUTE_MEMORY_MANAGER_HPP
