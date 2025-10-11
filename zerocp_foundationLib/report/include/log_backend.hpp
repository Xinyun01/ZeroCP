#ifndef LOG_BACKEND_HPP
#define LOG_BACKEND_HPP

#include <thread>
#include <atomic>
#include <string>
#include "lockfree_ringbuffer.hpp"

// 前向声明
namespace ZeroCP {
namespace Log {
    class LogStream;
}
}

namespace ZeroCP
{
namespace Log
{

/// @brief 日志后端（负责异步处理日志）
class LogBackend
{
public:
    LogBackend() noexcept;
    ~LogBackend() noexcept;

    /// @brief 启动后台线程
    void start() noexcept;
    
    /// @brief 停止后台线程
    void stop() noexcept;
    
    /// @brief 提交日志消息到队列（非阻塞，队列满时丢弃）
    void submitLog(const char* data, size_t len) noexcept;
    
    /// @brief 提交日志消息到队列（零拷贝版本）
    void submitLogZeroCopy(const char* data, size_t len) noexcept;
    
    /// @brief 提交日志消息到队列（阻塞版本，保证不丢失）
    /// @param data 日志数据
    /// @param len 数据长度
    /// @param timeout_us 超时时间（微秒），0表示无限等待
    /// @return true=成功, false=超时
    bool submitLogBlocking(const char* data, size_t len, uint64_t timeout_us = 0) noexcept;
    
    /// @brief 获取丢弃的日志数量
    uint64_t getDroppedCount() const noexcept { return dropped_count_.load(); }
    
    /// @brief 获取已处理的日志数量
    uint64_t getProcessedCount() const noexcept { return processed_count_.load(); }

    // 禁止拷贝和移动
    LogBackend(const LogBackend&) = delete;
    LogBackend(LogBackend&&) = delete;
    LogBackend& operator=(const LogBackend&) = delete;
    LogBackend& operator=(LogBackend&&) = delete;

private:
    /// @brief 后台工作线程函数
    void workerThread() noexcept;
    
    /// @brief 处理单条日志消息
    void processLogMessage(const LogMessage& msg) noexcept;
    
    /// @brief 刷新缓冲区
    void flush() noexcept;

    // 数据成员
    std::thread worker_thread;                      // 后台工作线程
    std::atomic<bool> running_{false};                          // 运行标志
    std::atomic<uint64_t> dropped_count_{0};                    // 丢弃的日志数量
    std::atomic<uint64_t> processed_count_{0};                  // 已处理的日志数量
    LockFreeRingBuffer<LogMessage, LOG_QUEUE_CAPACITY> ring_buffer_;   // 无锁队列
};

} // namespace Log
} // namespace ZeroCP

#endif // LOG_BACKEND_HPP
