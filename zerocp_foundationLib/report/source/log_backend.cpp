#include "log_backend.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <cstring>
#include <algorithm>

namespace ZeroCP {
namespace Log {

// 构造函数
LogBackend::LogBackend() noexcept
{
    // 初始化为未运行状态
    // running_ 使用默认初始化（在头文件中已设为 false）
}

// 析构函数
LogBackend::~LogBackend() noexcept
{   
    // 确保停止后台线程
    stop();
}

// 启动后台线程
void LogBackend::start() noexcept
{
    running_.store(true, std::memory_order_release);
    worker_thread = std::thread(&LogBackend::workerThread, this);
}

// 停止后台线程
void LogBackend::stop() noexcept
{
    running_.store(false, std::memory_order_release);
    if (worker_thread.joinable()) 
    {
        worker_thread.join();
    }
}

// 提交日志消息到队列
void LogBackend::submitLog(const char* data, size_t len) noexcept
{
    try {
        // 1. 先检查队列是否已满，避免不必要的消息构造
        if (ring_buffer_.isFull()) {
            dropped_count_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        
        // 2. 创建 LogMessage 对象
        LogMessage msg;
        
        // 3. 直接设置消息内容，避免字符串复制
        if (len > 0 && data != nullptr) {
            size_t copy_len = std::min(len, LogMessage::MAX_MESSAGE_SIZE - 1);
            memcpy(msg.message, data, copy_len);
            msg.message[copy_len] = '\0';
            msg.length = copy_len;
        }
        
        // 4. 尝试推入队列
        if (!ring_buffer_.tryPush(msg)) {
            // 队列满了（可能在检查后被其他线程填满），增加丢弃计数
            dropped_count_.fetch_add(1, std::memory_order_relaxed);
        }
    } catch (...) {
        // 日志系统不应该抛出异常
        dropped_count_.fetch_add(1, std::memory_order_relaxed);
    }
}

// 工作线程函数
void LogBackend::workerThread() noexcept
{
    // 创建临时变量用于接收消息
    LogMessage msg;
    
    // 主循环：只要 running_ 为 true 就继续运行
    while (running_.load(std::memory_order_acquire)) {
        // 尝试从队列中取出一条消息
        if (ring_buffer_.tryPop(msg)) {
            // 成功取到消息，处理它
            processLogMessage(msg);
            
            // 增加已处理计数
            processed_count_.fetch_add(1, std::memory_order_relaxed);
        } else {
            // 队列为空，休眠一段时间避免 CPU 空转
            // 100 微秒是一个平衡点：既不会太频繁唤醒，也不会延迟太大
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    
    // 退出循环后，处理剩余的消息
    // 这样可以确保在停止时不会丢失队列中的消息
    while (ring_buffer_.tryPop(msg)) {
        processLogMessage(msg);
        processed_count_.fetch_add(1, std::memory_order_relaxed);
    }
}

// 处理单条日志消息
void LogBackend::processLogMessage(const LogMessage& msg) noexcept
{
    try {
        // 直接输出到控制台，避免创建临时 string 对象
        if (msg.length > 0) {
            std::cout.write(msg.message, msg.length);
            std::flush(std::cout);
        }
    } catch (...) {
        // 日志系统不应该抛出异常
    }
}

// 刷新输出缓冲区
void LogBackend::flush() noexcept
{
    try {
        std::cout.flush();
    } catch (...) {
        // 忽略异常
    }
}

} // namespace Log
} // namespace ZeroCP