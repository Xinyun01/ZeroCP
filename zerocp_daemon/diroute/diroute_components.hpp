#ifndef ZEROCP_DIROUTE_COMPONENTS_HPP
#define ZEROCP_DIROUTE_COMPONENTS_HPP

#include "zerocp_daemon/memory/include/heartbeat_pool.hpp"
#include <type_traits>
#include <new>
#include <cstdint>
#include <cstddef>

namespace ZeroCP
{
namespace Diroute
{

/// Diroute 组件容器（iceoryx 分布式构造模式）
/// 先预留内存，再分步构造，显式管理生命周期
struct DirouteComponents
{
    // 预留心跳池内存（未构造）- 使用 C++23 推荐的 alignas 替代废弃的 aligned_storage_t
    alignas(alignof(zerocp::memory::HeartbeatPool)) 
    std::byte m_heartbeatPoolStorage[sizeof(zerocp::memory::HeartbeatPool)];
    
    // 构造状态标志
    bool m_heartbeatPoolConstructed{false};
    
    // 默认构造函数：只预留内存，不构造对象
    DirouteComponents() noexcept = default;
    
    // 使用 placement new 构造心跳池
    zerocp::memory::HeartbeatPool& constructHeartbeatPool() noexcept
    {
        if (!m_heartbeatPoolConstructed)
        {
            new (&m_heartbeatPoolStorage) zerocp::memory::HeartbeatPool();
            m_heartbeatPoolConstructed = true;
        }
        return *reinterpret_cast<zerocp::memory::HeartbeatPool*>(&m_heartbeatPoolStorage);
    }
    
    // 获取心跳池引用（必须先调用 constructHeartbeatPool）
    zerocp::memory::HeartbeatPool& heartbeatPool() noexcept
    {
        return *reinterpret_cast<zerocp::memory::HeartbeatPool*>(&m_heartbeatPoolStorage);
    }
    
    const zerocp::memory::HeartbeatPool& heartbeatPool() const noexcept
    {
        return *reinterpret_cast<const zerocp::memory::HeartbeatPool*>(&m_heartbeatPoolStorage);
    }
    
    // 检查心跳池是否已构造
    [[nodiscard]] bool isHeartbeatPoolConstructed() const noexcept
    {
        return m_heartbeatPoolConstructed;
    }
    
    // 析构函数：按 LIFO 顺序显式销毁已构造的组件
    ~DirouteComponents() noexcept
    {
        if (m_heartbeatPoolConstructed)
        {
            reinterpret_cast<zerocp::memory::HeartbeatPool*>(&m_heartbeatPoolStorage)->~HeartbeatPool();
            m_heartbeatPoolConstructed = false;
        }
    }
    
    // 禁止拷贝和移动
    DirouteComponents(const DirouteComponents&) = delete;
    DirouteComponents& operator=(const DirouteComponents&) = delete;
    DirouteComponents(DirouteComponents&&) = delete;
    DirouteComponents& operator=(DirouteComponents&&) = delete;
};

}
}
#endif
