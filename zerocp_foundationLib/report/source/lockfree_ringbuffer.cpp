#include "lockfree_ringbuffer.hpp"
#include <algorithm>

// 注意：LockFreeRingBuffer 是模板类，所有实现都在 .inl 文件中
// 这个 .cpp 文件用于实现 LogMessage 的成员函数

namespace ZeroCP
{
namespace Log
{

// ========== LogMessage 实现 ==========

// 拷贝构造函数（优化版：只拷贝实际长度）
LogMessage::LogMessage(const LogMessage& other) noexcept
    : length(other.length)
{
    if (other.length > 0) {
        std::memcpy(this->message, other.message, other.length);
    }
}

// 拷贝赋值运算符（优化版：只拷贝实际长度）
LogMessage& LogMessage::operator=(const LogMessage& other) noexcept
{
    if (this != &other) {  // 防止自赋值
        // 只拷贝实际使用的数据
        if (other.length > 0) {
            std::memcpy(this->message, other.message, other.length);
        }
        this->length = other.length;
    }
    return *this;
}

void LogMessage::setMessage(const std::string& msg) noexcept
{
    // 计算实际要复制的长度（不超过最大长度-1，留一个位置给 '\0'）
    length = std::min(msg.length(), MAX_MESSAGE_SIZE - 1);
    
    // 复制消息内容
    std::memcpy(message, msg.c_str(), length);
    
    // 添加字符串结束符
    message[length] = '\0';
}

std::string LogMessage::getMessage() const noexcept
{
    // 使用 length 来构造字符串，避免依赖 '\0'
    return std::string(message, length);
}

} // namespace Log
} // namespace ZeroCP