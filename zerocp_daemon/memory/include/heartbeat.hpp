#ifndef ZEROCP_HEARTBEAT_HPP
#define ZEROCP_HEARTBEAT_HPP

#include <atomic>
#include <chrono>
#include <cstdint>

namespace zerocp::memory
{

/// 心跳槽位：存储在共享内存中的原子时间戳
/// 守护进程和应用进程都可以无锁并发更新
struct HeartbeatSlot
{
    using Timestamp = uint64_t;

    HeartbeatSlot() noexcept = default;
    HeartbeatSlot(const HeartbeatSlot&) = delete;
    HeartbeatSlot& operator=(const HeartbeatSlot&) = delete;

    /// 存储指定的时间戳（纳秒）
    void store(Timestamp ts) noexcept
    {
        m_lastTimestamp.store(ts, std::memory_order_release);
    }

    /// 获取当前时间并存储（默认使用 steady_clock）
    template <typename Clock = std::chrono::steady_clock>
    void touch(typename Clock::time_point tp = Clock::now()) noexcept
    {
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
        store(static_cast<Timestamp>(ns));
    }

    /// 读取上次写入的时间戳
    [[nodiscard]] Timestamp load() const noexcept
    {
        return m_lastTimestamp.load(std::memory_order_acquire);
    }

  private:
    std::atomic<Timestamp> m_lastTimestamp{0};
};

} // namespace zerocp::memory

#endif // ZEROCP_HEARTBEAT_HPP
