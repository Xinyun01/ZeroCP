#ifndef ZEROCP_VOCABULARY_DETAIL_STRING_INL
#define ZEROCP_VOCABULARY_DETAIL_STRING_INL
#include "string.hpp"
#include <cstring>
#include <algorithm> // std::copy等算法
#include <cstdint>   // 标准整数类型
#include <cstring>   // C字符串操作
#include "logging.hpp"

namespace ZeroCP
{
// 引入日志级别枚举
using Log::LogLevel;

// 默认构造函数
template<uint64_t Capacity>
inline constexpr string<Capacity>::string() noexcept
    : m_string{}, m_strSize(0U)
{
    m_string[0] = '\0';
}

template<uint64_t Capacity>
inline string<Capacity>::string(const string& other) noexcept
{
    copy(other);
}

template<uint64_t Capacity>
inline string<Capacity>::string(string&& other) noexcept
{
    move(std::move(other));
}

// 拷贝赋值运算符
template<uint64_t Capacity>
inline string<Capacity>& string<Capacity>::operator=(const string& other) noexcept
{
    if (this != &other) {
        copy(other);
    }
    return *this;
}

// 移动赋值运算符
template<uint64_t Capacity>
inline string<Capacity>& string<Capacity>::operator=(string&& other) noexcept
{
    if (this != &other) {
        move(std::move(other));
    }
    return *this;
}

// 析构函数
template<uint64_t Capacity>
inline string<Capacity>::~string() noexcept
{
    // 栈上对象，无需手动释放
}

template<uint64_t Capacity>
inline constexpr uint64_t string<Capacity>::size() const noexcept
{
    return m_strSize;
}

// 拷贝另一个固定容量字符串的内容到本字符串
template<uint64_t Capacity>
template<uint64_t N>
inline string<Capacity>& string<Capacity>::copy(const string<N>& rhs) noexcept
{
    // 保证拷贝对象不会溢出目标容量
    static_assert(N <= Capacity, "The capacity of the fixed string must be greater than the length of the string to be copied!");
    uint64_t strSize = rhs.size();
    // 拷贝内容
    std::memcpy(m_string, rhs.c_str(), static_cast<size_t>(strSize));
    // 添加字符串的结尾符
    m_string[strSize] = '\0';
    // 设置长度信息
    m_strSize = strSize;
    return *this;
}

template<uint64_t Capacity>
template<uint64_t N>
inline string<Capacity>& string<Capacity>::move(string<N>&& rhs) noexcept
{
    // 保证移动对象不会溢出目标容量
    static_assert(N <= Capacity, "The capacity of the fixed string must be greater than the length of the string to be copied!");
    const uint64_t strSize{rhs.size()};
    std::memcpy(m_string, rhs.c_str(), static_cast<size_t>(strSize));
    m_string[strSize] = '\0';
    m_strSize = strSize;
    rhs.clear();
    return *this;
}

template<uint64_t Capacity>
inline const char* string<Capacity>::c_str() const noexcept
{
    return &m_string[0];
}

template<uint64_t Capacity>
inline uint64_t string<Capacity>::capacity() const noexcept
{
    return Capacity;
}

template<uint64_t Capacity>
inline string<Capacity>& string<Capacity>::insert(uint64_t pos, const char* str) noexcept
{
    
    // 运行时检查：确保不会溢出
    const uint64_t strLen = strlen(str);
    
    // 如果插入位置超出当前大小，就追加到末尾
    if(pos >= m_strSize)
    {
        pos = m_strSize;
    }
    
    // 确保插入后不会超出容量
    const uint64_t newSize = m_strSize + strLen;
    if(newSize > Capacity)
    {
        // 容量不足，不执行插入
        ZEROCP_LOG(Error, "String insert failed: capacity overflow! Current size: " 
                   << m_strSize << ", trying to insert: " << strLen 
                   << ", capacity: " << Capacity);
        return *this;
    }
    
    if(pos == m_strSize)
    {
        // 追加到末尾
        std::memcpy(&m_string[pos], str, static_cast<size_t>(strLen));
        m_strSize = newSize;
        m_string[m_strSize] = '\0';
    }
    else
    {
        // 在中间插入，需要移动后面的内容
        std::memmove(&m_string[pos + strLen], &m_string[pos], m_strSize - pos);
        std::memcpy(&m_string[pos], str, static_cast<size_t>(strLen));
        m_strSize = newSize;
        m_string[m_strSize] = '\0';
    }
    return *this;
}

// 编译时检查版本：接受字符串字面量
template<uint64_t Capacity>
template<uint64_t N>
inline string<Capacity>& string<Capacity>::insert(uint64_t pos, const char (&str)[N]) noexcept
{
    // N 包含了 '\0'，所以实际字符串长度是 N-1
    constexpr uint64_t strLen = N - 1;
    
    // 编译时检查：确保字符串长度不超过容量
    static_assert(strLen <= Capacity, "The string to be inserted is too long for this fixed string capacity!");
    
    // 如果插入位置超出当前大小，就追加到末尾
    if(pos >= m_strSize)
    {
        pos = m_strSize;
    }
    
    // 运行时检查：确保插入后不会超出容量
    // 注意：这里虽然有编译时检查 strLen <= Capacity，
    // 但 m_strSize + strLen 可能仍然超过 Capacity
    const uint64_t newSize = m_strSize + strLen;
    if(newSize > Capacity)
    {
        // 容量不足，不执行插入
        ZEROCP_LOG(Error, "String insert failed: capacity overflow! Current size: " 
                   << m_strSize << ", trying to insert: " << strLen 
                   << ", capacity: " << Capacity);
        return *this;
    }
    
    if(pos == m_strSize)
    {
        // 追加到末尾
        std::memcpy(&m_string[pos], str, static_cast<size_t>(strLen));
        m_strSize = newSize;
        m_string[m_strSize] = '\0';
    }
    else
    {
        // 在中间插入，需要移动后面的内容
        std::memmove(&m_string[pos + strLen], &m_string[pos], m_strSize - pos);
        std::memcpy(&m_string[pos], str, static_cast<size_t>(strLen));
        m_strSize = newSize;
        m_string[m_strSize] = '\0';
    }
    return *this;
}


template<uint64_t Capacity>
inline bool string<Capacity>::empty() const noexcept
{
    return m_strSize == 0U;
}

template<uint64_t Capacity>
inline void string<Capacity>::clear() noexcept
{
    m_strSize = 0U;
    m_string[0] = '\0';
}

}
#endif