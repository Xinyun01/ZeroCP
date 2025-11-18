#ifndef ZEROCP_DIROUTE_COMPONENTS_HPP
#define ZEROCP_DIROUTE_COMPONENTS_HPP

#include "zerocp_daemon/memory/include/heartbeat_pool.hpp"
#include <type_traits>
#include <new>
#include <cstdint>

namespace ZeroCP
{
namespace Diroute
{

/// @brief DirouteComponents follows iceoryx distributed construction pattern:
/// 1. Pre-allocate memory using aligned_storage (reserve resources)
/// 2. Use placement new to construct components step-by-step
/// 3. Explicit lifecycle management (construct/destruct)
struct DirouteComponents
{
    // =========================================================================
    // Step 1: Memory Layout - Reserve aligned storage for each component
    // =========================================================================
    
    /// @brief Reserved memory for HeartbeatPool (not yet constructed)
    /// Uses std::aligned_storage_t to reserve properly aligned memory without construction
    using HeartbeatPoolStorage = std::aligned_storage_t<
        sizeof(zerocp::memory::HeartbeatPool),
        alignof(zerocp::memory::HeartbeatPool)>;
    HeartbeatPoolStorage m_heartbeatPoolStorage;
    
    // TODO: Add MemPoolManager and PortManager storage when those components are implemented
    
    // =========================================================================
    // Step 2: Construction State Tracking
    // =========================================================================
    
    /// @brief Track whether HeartbeatPool has been constructed
    bool m_heartbeatPoolConstructed{false};
    
    // =========================================================================
    // Step 3: Distributed Construction Methods (iceoryx pattern)
    // =========================================================================
    
    /// @brief Default constructor - only reserves memory, does NOT construct objects
    DirouteComponents() noexcept = default;
    
    /// @brief Construct HeartbeatPool using placement new
    /// @return Reference to the constructed HeartbeatPool
    /// @note This is the first component to construct (similar to iceoryx::runtime)
    zerocp::memory::HeartbeatPool& constructHeartbeatPool() noexcept
    {
        if (!m_heartbeatPoolConstructed)
        {
            // Placement new: construct object in pre-allocated memory
            // This is the key pattern from iceoryx - explicit construction in reserved memory
            new (&m_heartbeatPoolStorage) zerocp::memory::HeartbeatPool();
            m_heartbeatPoolConstructed = true;
        }
        return *reinterpret_cast<zerocp::memory::HeartbeatPool*>(&m_heartbeatPoolStorage);
    }
    
    // TODO: Add constructMemPoolManager() and constructPortManager() when those components exist
    
    // =========================================================================
    // Step 4: Access Methods (only valid after construction)
    // =========================================================================
    
    /// @brief Get reference to HeartbeatPool (must be constructed first)
    /// @note Only call this after constructHeartbeatPool() has been called
    zerocp::memory::HeartbeatPool& heartbeatPool() noexcept
    {
        return *reinterpret_cast<zerocp::memory::HeartbeatPool*>(&m_heartbeatPoolStorage);
    }
    
    const zerocp::memory::HeartbeatPool& heartbeatPool() const noexcept
    {
        return *reinterpret_cast<const zerocp::memory::HeartbeatPool*>(&m_heartbeatPoolStorage);
    }
    
    /// @brief Check if HeartbeatPool has been constructed
    [[nodiscard]] bool isHeartbeatPoolConstructed() const noexcept
    {
        return m_heartbeatPoolConstructed;
    }
    
    // TODO: Add memPoolManager() and portManager() access methods when those components exist
    
    // =========================================================================
    // Step 5: Destruction (explicitly destroy constructed components)
    // =========================================================================
    
    /// @brief Destructor - manually destroy constructed components in reverse order
    /// @note This follows iceoryx pattern: explicit destruction in LIFO order
    ~DirouteComponents() noexcept
    {
        // Destroy HeartbeatPool if it was constructed
        // When more components are added, destroy them in reverse construction order (LIFO)
        if (m_heartbeatPoolConstructed)
        {
            // Explicit destructor call for objects constructed with placement new
            reinterpret_cast<zerocp::memory::HeartbeatPool*>(&m_heartbeatPoolStorage)->~HeartbeatPool();
            m_heartbeatPoolConstructed = false;
        }
    }
    
    // Delete copy and move (shared memory components should not be copied/moved)
    DirouteComponents(const DirouteComponents&) = delete;
    DirouteComponents& operator=(const DirouteComponents&) = delete;
    DirouteComponents(DirouteComponents&&) = delete;
    DirouteComponents& operator=(DirouteComponents&&) = delete;
};

}
}
#endif
