#include "logsteam.hpp"
#include "logging.hpp"
#include "log_backend.hpp"
#include <cstring>
#include <cstdio>
#include <chrono>
#include <ctime>

namespace ZeroCP {
namespace Log {

// 固定缓冲区大小
constexpr size_t MAX_LOG_MESSAGE_SIZE = 512;  // 单条日志最大 0.5KB

// LogStream 的内部实现
    class LogStream::Impl {
    public:
    const char* file_;
    int line_;
    const char* function_;
    LogLevel level_;
    char buffer_[MAX_LOG_MESSAGE_SIZE];  // 固定大小缓冲区
    size_t current_pos_;                  // 当前写入位置

    Impl(const char* file, int line, const char* function, LogLevel level)
        : file_(file), line_(line), function_(function), level_(level), current_pos_(0)
    {
        buffer_[0] = '\0';  // 初始化为空字符串
    }

    // 安全地追加字符串到缓冲区
    void append(const char* str, size_t len) noexcept
    {
        if (!str || len == 0) return;
        
        size_t available = MAX_LOG_MESSAGE_SIZE - current_pos_ - 1;  // 保留 1 字节给 '\0'
        size_t to_copy = (len < available) ? len : available;
        
        if (to_copy > 0) {
            memcpy(buffer_ + current_pos_, str, to_copy);
            current_pos_ += to_copy;
            buffer_[current_pos_] = '\0';
        }
    }

    void append(const char* str) noexcept
    {
        if (str) {
            append(str, strlen(str));
        }
    }
};

// 构造函数
LogStream::LogStream(const char* file, int line, const char* function, LogLevel logLevel) noexcept
    : impl_(std::make_unique<Impl>(file, line, function, logLevel))
{
}

// 析构函数 - 在这里提交日志
LogStream::~LogStream() noexcept
{
    // 完全使用 Impl 的 buffer_，零额外内存分配
    char* buffer = impl_->buffer_;
    size_t& pos = impl_->current_pos_;
    
    // 将用户内容向后移动，为前缀信息腾出空间
    size_t user_len = pos;
    if (user_len > 0) {
        // 向后移动用户内容，为前缀预留足够空间（约100字节）
        const size_t prefix_space = 100;
        if (user_len + prefix_space < MAX_LOG_MESSAGE_SIZE) {
            memmove(buffer + prefix_space, buffer, user_len);
        }
    }
    
    // 重置位置，开始构建完整日志
    pos = 0;
    buffer[0] = '\0';

    // 1. 获取当前时间戳
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);

    // 2. 格式化时间戳: [2025-10-10 22:30:45.123]
    int n = snprintf(buffer + pos, MAX_LOG_MESSAGE_SIZE - pos,
                     "[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
                     tm_now.tm_year + 1900,
                     tm_now.tm_mon + 1,
                     tm_now.tm_mday,
                     tm_now.tm_hour,
                     tm_now.tm_min,
                     tm_now.tm_sec,
                     static_cast<int>(ms.count()));
    
    if (n > 0) pos += n;

    // 3. 添加日志级别
    const char* level_str = "";
    switch (impl_->level_) {
        case LogLevel::Off:     level_str = "OFF  "; break;
        case LogLevel::Fatal:   level_str = "FATAL"; break;
        case LogLevel::Error:   level_str = "ERROR"; break;
        case LogLevel::Warn:    level_str = "WARN "; break;
        case LogLevel::Info:    level_str = "INFO "; break;
        case LogLevel::Debug:   level_str = "DEBUG"; break;
        case LogLevel::Trace:   level_str = "TRACE"; break;
    }

    n = snprintf(buffer + pos, MAX_LOG_MESSAGE_SIZE - pos, "[%s] ", level_str);
    if (n > 0) pos += n;

    // 4. 添加文件位置信息（只取文件名，不含路径）
    const char* filename = impl_->file_;
    const char* last_slash = strrchr(filename, '/');
    if (last_slash) {
        filename = last_slash + 1;
    }

    n = snprintf(buffer + pos, MAX_LOG_MESSAGE_SIZE - pos,
                 "[%s:%d] ", filename, impl_->line_);
    if (n > 0) pos += n;

    // 5. 添加用户的日志内容（已经在 buffer + 100 位置）
    size_t available = MAX_LOG_MESSAGE_SIZE - pos - 2;  // 保留 2 字节给 '\n' 和 '\0'
    size_t content_len = user_len;
    if (content_len > available) {
        content_len = available;
    }
    
    if (content_len > 0) {
        memcpy(buffer + pos, buffer + 100, content_len);  // 从移动后的位置复制
        pos += content_len;
    }

    // 6. 确保以换行符结尾
    if (pos == 0 || buffer[pos - 1] != '\n') {
        if (pos < MAX_LOG_MESSAGE_SIZE - 1) {
            buffer[pos++] = '\n';
        }
    }
    buffer[pos] = '\0';

    // 7. 提交到后端的无锁队列（直接传递原始数据，避免字符串复制）
    Log_Manager::getInstance().getBackend().submitLog(buffer, pos);
}

// 重载运算符以处理各种类型
LogStream& LogStream::operator<<(const char* str) noexcept
{
    impl_->append(str);
    return *this;
}

LogStream& LogStream::operator<<(const std::string& str) noexcept
{
    impl_->append(str.c_str(), str.length());
    return *this;
}

LogStream& LogStream::operator<<(int value) noexcept
{
    // 直接使用 Impl 的 buffer_ 进行格式化，避免额外内存分配
    char temp[32];
    int n = snprintf(temp, sizeof(temp), "%d", value);
    if (n > 0) {
        impl_->append(temp, static_cast<size_t>(n));
    }
    return *this;
}

LogStream& LogStream::operator<<(unsigned int value) noexcept
{
    static thread_local char temp[32];
    int n = snprintf(temp, sizeof(temp), "%u", value);
    if (n > 0) {
        impl_->append(temp, static_cast<size_t>(n));
    }
    return *this;
}

LogStream& LogStream::operator<<(long value) noexcept
{
    static thread_local char temp[32];
    int n = snprintf(temp, sizeof(temp), "%ld", value);
    if (n > 0) {
        impl_->append(temp, static_cast<size_t>(n));
    }
    return *this;
}

LogStream& LogStream::operator<<(unsigned long value) noexcept
{
    static thread_local char temp[32];
    int n = snprintf(temp, sizeof(temp), "%lu", value);
    if (n > 0) {
        impl_->append(temp, static_cast<size_t>(n));
    }
    return *this;
}

LogStream& LogStream::operator<<(double value) noexcept
{
    static thread_local char temp[64];
    int n = snprintf(temp, sizeof(temp), "%g", value);
    if (n > 0) {
        impl_->append(temp, static_cast<size_t>(n));
    }
    return *this;
}

LogStream& LogStream::operator<<(bool value) noexcept
{
    impl_->append(value ? "true" : "false");
    return *this;
}

} // namespace Log
} // namespace ZeroCP

