#ifndef ZEROCP_DIROUTE_COMPONENTS_HPP
#define ZEROCP_DIROUTE_COMPONENTS_HPP

#include "zerocp_daemon/memory/include/heartbeat_pool.hpp"
#include "zerocp_daemon/communication/include/popo/message_header.hpp"
#include "zerocp_foundationLib/report/include/lockfree_ringbuffer.hpp"
#include <type_traits>
#include <new>
#include <cstdint>
#include <cstddef>
#include <array>
#include <atomic>
#include <optional>
#include <limits>

namespace ZeroCP
{
namespace Diroute
{

/// Diroute 组件容器（iceoryx 分布式构造模式）
/// 先预留内存，再分步构造，显式管理生命周期
struct DirouteComponents
{
    using MessageQueue = ZeroCP::Log::LockFreeRingBuffer<ZeroCP::Popo::MessageHeader, 1024>;
    static constexpr uint32_t MAX_RECEIVE_QUEUES = 64;

    // 预留心跳池内存（未构造）- 使用 C++23 推荐的 alignas 替代废弃的 aligned_storage_t
    alignas(alignof(zerocp::memory::HeartbeatPool)) 
    std::byte m_heartbeatPoolStorage[sizeof(zerocp::memory::HeartbeatPool)];
    
    // 构造状态标志
    bool m_heartbeatPoolConstructed{false};

    struct ReceiveQueueSlot
    {
        alignas(alignof(MessageQueue)) std::byte storage[sizeof(MessageQueue)];
        std::atomic<bool> inUse{false};
    };

    std::array<ReceiveQueueSlot, MAX_RECEIVE_QUEUES> m_receiveQueues{};
    bool m_receiveQueuesConstructed{false};
    void* m_baseAddress{nullptr};
    
    // 默认构造函数：只预留内存，不构造对象
    DirouteComponents() noexcept = default;
    
    void setBaseAddress(void* base) noexcept
    {
        m_baseAddress = base;
    }

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

    void constructReceiveQueues() noexcept
    {
        if (m_receiveQueuesConstructed)
        {
            return;
        }

        for (auto& slot : m_receiveQueues)
        {
            new (&slot.storage) MessageQueue();
            slot.inUse.store(false, std::memory_order_relaxed);
        }
        m_receiveQueuesConstructed = true;
    }

    void initializeQueueDescriptors() noexcept
    {
        for (auto& slot : m_receiveQueues)
        {
            slot.inUse.store(false, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] std::optional<uint32_t> acquireQueue() noexcept
    {
        for (uint32_t index = 0; index < m_receiveQueues.size(); ++index)
        {
            bool expected = false;
            if (m_receiveQueues[index].inUse.compare_exchange_strong(expected,
                                                                     true,
                                                                     std::memory_order_acq_rel))
            {
                return index;
            }
        }
        return std::nullopt;
    }

    void releaseQueue(uint32_t index) noexcept
    {
        if (index >= m_receiveQueues.size())
        {
            return;
        }
        m_receiveQueues[index].inUse.store(false, std::memory_order_release);
    }

    [[nodiscard]] uint64_t getQueueOffset(uint32_t index) const noexcept
    {
        if (index >= m_receiveQueues.size() || m_baseAddress == nullptr)
        {
            return 0;
        }

        auto* queuePtr = reinterpret_cast<const MessageQueue*>(m_receiveQueues[index].storage);
        auto* base = static_cast<const char*>(m_baseAddress);
        auto* queueAddr = reinterpret_cast<const char*>(queuePtr);
        return static_cast<uint64_t>(queueAddr - base);
    }

    [[nodiscard]] MessageQueue* getQueueByOffset(uint64_t offset) noexcept
    {
        if (m_baseAddress == nullptr)
        {
            return nullptr;
        }

        auto* base = static_cast<char*>(m_baseAddress);
        return reinterpret_cast<MessageQueue*>(base + offset);
    }
    
    // 析构函数：按 LIFO 顺序显式销毁已构造的组件
    ~DirouteComponents() noexcept
    {
        if (m_heartbeatPoolConstructed)
        {
            reinterpret_cast<zerocp::memory::HeartbeatPool*>(&m_heartbeatPoolStorage)->~HeartbeatPool();
            m_heartbeatPoolConstructed = false;
        }

        if (m_receiveQueuesConstructed)
        {
            for (auto& slot : m_receiveQueues)
            {
                reinterpret_cast<MessageQueue*>(&slot.storage)->~MessageQueue();
            }
            m_receiveQueuesConstructed = false;
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
