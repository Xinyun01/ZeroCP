#ifndef ZEROCP_HEARTBEAT_POOL_HPP
#define ZEROCP_HEARTBEAT_POOL_HPP

#include "heartbeat.hpp"
#include "zerocp_foundationLib/vocabulary/include/fixed_position_container.hpp"

namespace zerocp::memory
{

/// 心跳池：固定大小的心跳槽位数组（容量 100）
/// 基于 FixedPositionContainer，槽位地址稳定，适合共享内存
class HeartbeatPool
{
  public:
    static constexpr uint64_t kMaxHeartbeats = 100;
    using Container = ZeroCP::FixedPositionContainer<HeartbeatSlot, kMaxHeartbeats>;
    using Iterator = typename Container::Iterator;
    using ConstIterator = typename Container::ConstIterator;

    HeartbeatPool() noexcept = default;
    HeartbeatPool(const HeartbeatPool&) = delete;
    HeartbeatPool& operator=(const HeartbeatPool&) = delete;

    /// 在槽位池中构造一个新的心跳槽位，如果槽位池已满则返回 end()
    [[nodiscard]] Iterator emplace() noexcept
    {
        return m_slots.emplace();
    }

    /// 释放一个已分配的槽位
    void release(Iterator it) noexcept
    {
        if (it != m_slots.end())
        {
            m_slots.erase(it);
        }
    }

    /// 遍历所有已分配的槽位（例如：看门狗检查）
    template <typename Fn>
    void for_each(Fn&& fn) noexcept
    {
        for (auto it = m_slots.begin(); it != m_slots.end(); ++it)
        {
            fn(*it);
        }
    }

    /// 获取已分配的槽位数量
    [[nodiscard]] uint64_t size() const noexcept
    {
        return m_slots.size();
    }

    /// 获取最大容量
    [[nodiscard]] constexpr uint64_t capacity() const noexcept
    {
        return kMaxHeartbeats;
    }

    /// 检查槽位池是否已满
    [[nodiscard]] bool isFull() const noexcept
    {
        return m_slots.full();
    }

    /// 检查槽位池是否为空
    [[nodiscard]] bool isEmpty() const noexcept
    {
        return m_slots.empty();
    }

    // 迭代器接口
    [[nodiscard]] Iterator begin() noexcept { return m_slots.begin(); }
    [[nodiscard]] Iterator end() noexcept { return m_slots.end(); }
    [[nodiscard]] ConstIterator begin() const noexcept { return m_slots.begin(); }
    [[nodiscard]] ConstIterator end() const noexcept { return m_slots.end(); }
    [[nodiscard]] ConstIterator cbegin() const noexcept { return m_slots.cbegin(); }
    [[nodiscard]] ConstIterator cend() const noexcept { return m_slots.cend(); }

  private:
    Container m_slots{};
};

} // namespace zerocp::memory

#endif // ZEROCP_HEARTBEAT_POOL_HPP
