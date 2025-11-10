#ifndef ZEROCP_VOCABULARY_STRING_HPP
#define ZEROCP_VOCABULARY_STRING_HPP

#include <cstdint>   // 标准整数类型
#include <cstring>   // C字符串操作
namespace ZeroCP
{


/// @brief 固定容量字符串实现，不能抛异常且不能用堆
template <uint64_t Capacity>
class string
{
    static_assert(Capacity > 0U, "The capacity of the fixed string must be greater than 0!");
public:

    constexpr string() noexcept;

    string(const string& other) noexcept;

    string(string&& other) noexcept;
    
    string& operator=(const string& other) noexcept;

    string& operator=(string&& other) noexcept;

    ~string() noexcept;

    constexpr uint64_t size() const noexcept;
    
    const char* c_str() const noexcept;

    uint64_t capacity() const noexcept;

    // 运行时检查版本：接受 const char* 指针
    string& insert(uint64_t pos, const char* str) noexcept;
    
    // 编译时检查版本：接受字符串字面量，可以在编译时检查大小
    template<uint64_t N>
    string& insert(uint64_t pos, const char (&str)[N]) noexcept;
    
    bool empty() const noexcept;

    void clear() noexcept;
private:

    template<uint64_t N>
    string& copy(const string<N>& rhs) noexcept;

    template <uint64_t N>
    string& move(string<N>&& rhs) noexcept;

    char m_string[Capacity+1] = {};
    uint64_t m_strSize = 0U;
};

}

// 包含实现文件
#include "zerocp_foundationLib/vocabulary/detail/string.inl"

#endif