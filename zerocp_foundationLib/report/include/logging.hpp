#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <cstdint>
#include <atomic>
#include <memory>
#include <string>

// 前向声明
namespace ZeroCP {
namespace Log {
    class LogBackend;
}
}
namespace ZeroCP
{
namespace Log
{

// 前向声明
class LogStream;
class LogBackend;

/// @brief 日志级别枚举
enum class LogLevel : uint8_t
{
    Off = 0,      // 关闭日志
    Fatal = 1,    // 致命错误（程序无法继续）
    Error = 2,    // 错误（严重但可恢复）
    Warn = 3,     // 警告（非预期但不严重）
    Info = 4,     // 信息（日常用户关心的）
    Debug = 5,    // 调试（开发者关心的）
    Trace = 6     // 追踪（详细调试信息）
};

/// @brief 日志管理器（单例模式）
/// @note 提示：使用单例模式确保全局只有一个实例
class Log_Manager
{
public:
    /// @brief 获取单例实例
    /// @note 提示：使用局部静态变量实现线程安全的单例
    static Log_Manager& getInstance() noexcept
    {
        static Log_Manager instance;
        return instance;
    }

    /// @brief 检查日志级别是否激活
    /// @note 提示：只有当消息级别不低于当前级别时才记录
    bool isLogLevelActive(LogLevel level) const noexcept
    {
        return current_level_ >= level;
    }

    /// @brief 设置当前日志级别
    void setLogLevel(LogLevel level) noexcept
    {
        current_level_ = level;
    }

    /// @brief 获取当前日志级别
    LogLevel getLogLevel() const noexcept
    {
        return current_level_;
    }

    /// @brief 获取日志后端
    LogBackend& getBackend() noexcept
    {
        return *backend_;
    }

    /// @brief 启动日志系统
    void start() noexcept;
    /// @brief 停止日志系统
    void stop() noexcept;

    // 禁止拷贝和移动
    Log_Manager(const Log_Manager&) = delete;
    Log_Manager(Log_Manager&&) = delete;
    Log_Manager& operator=(const Log_Manager&) = delete;
    Log_Manager& operator=(Log_Manager&&) = delete;

private:
    Log_Manager() noexcept;
    ~Log_Manager() noexcept;
    
    // 数据成员 数据等级
    std::atomic<LogLevel> current_level_{LogLevel::Info};
    std::unique_ptr<LogBackend> backend_;
};

} // namespace Log
} // namespace ZeroCP

// 包含 LogStream 完整定义以支持宏中的使用
#include "logsteam.hpp"
#include "log_backend.hpp"
/// @brief 日志记录宏
/// @param[in] level 用于日志消息的日志级别
/// @param[in] msg_stream 日志消息流；可以使用 '<<' 操作符记录多个项目
/// @note 提示：宏需要检查日志级别，如果激活则创建 LogStream 对象
/// @note 提示：使用 __FILE__, __LINE__, __FUNCTION__ 获取源码位置信息
#define ZEROCP_LOG(level, msg_stream) \
    do { \
        if (ZeroCP::Log::Log_Manager::getInstance().isLogLevelActive(ZeroCP::Log::LogLevel::level)) { \
            ZeroCP::Log::LogStream(__FILE__, __LINE__, static_cast<const char*>(__FUNCTION__), ZeroCP::Log::LogLevel::level) << msg_stream; \
        } \
    } while(0)

#endif // LOGGING_HPP
