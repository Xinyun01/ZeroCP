#ifndef LOGSTREAM_HPP
#define LOGSTREAM_HPP

#include "logging.hpp"
#include <string>
#include <memory>

namespace ZeroCP
{
namespace Log
{

/// @brief 日志流类 - 用于构建日志消息
/// @note 提示：在析构时将完整的日志消息提交到后端
class LogStream
{
public:
    /// @brief 构造函数
    /// @param file 源文件名
    /// @param line 行号
    /// @param function 函数名
    /// @param logLevel 日志级别
    LogStream(const char* file, int line, const char* function, LogLevel logLevel) noexcept;

    /// @brief 析构函数 - 在这里将日志消息提交到后端
    /// @note 提示：需要格式化消息（包含时间戳、文件名、行号等）并提交到后端
    ~LogStream() noexcept;

    // 禁止拷贝和移动
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;
    LogStream(LogStream&&) = delete;
    LogStream& operator=(LogStream&&) = delete;

    // ========== 重载运算符以处理各种类型 ==========
    
    /// @brief 重载运算符以处理C风格字符串
    LogStream& operator<<(const char* str) noexcept;

    /// @brief 重载运算符以处理std::string类型
    LogStream& operator<<(const std::string& str) noexcept;

    /// @brief 重载运算符以处理int类型
    LogStream& operator<<(int value) noexcept;

    /// @brief 重载运算符以处理unsigned int类型
    LogStream& operator<<(unsigned int value) noexcept;

    /// @brief 重载运算符以处理long类型
    LogStream& operator<<(long value) noexcept;

    /// @brief 重载运算符以处理unsigned long类型
    LogStream& operator<<(unsigned long value) noexcept;

    /// @brief 重载运算符以处理double类型
    LogStream& operator<<(double value) noexcept;

    /// @brief 重载运算符以处理bool类型
    LogStream& operator<<(bool value) noexcept;

private:
    // 使用 Pimpl 模式隐藏实现细节
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Log
} // namespace ZeroCP

#endif // LOGSTREAM_HPP
