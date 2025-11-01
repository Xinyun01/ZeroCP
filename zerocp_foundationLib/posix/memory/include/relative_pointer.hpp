#ifndef ZEROCP_POOL_REGISTRY_HPP
#define ZEROCP_POOL_REGISTRY_HPP

#include <cstdint>
#include <unordered_map>
#include <mutex>

namespace ZeroCP
{

// 池ID类型定义
using pool_id_t = std::uint64_t;

// 特殊池ID常量
constexpr pool_id_t MANAGEMENT_POOL_ID = 0;  // 管理区内存池ID

// 共享内存池注册表：存储每个pool_id对应的基地址
class PoolRegistry
{
};

template<typename T>
class RelativePointer
{
public:
        using ptr_t = T*;
        using offset_t = std::uint64_t;

        RelativePointer() noexcept = default;
        ~RelativePointer() noexcept = default;
        RelativePointer(ptr_t baseAddress, ptr_t const ptr, uint64_t pool_id) noexcept;
        RelativePointer(ptr_t ptr, uint64_t pool_id) noexcept;  // 简化构造函数，假设 baseAddress 为 nullptr
        RelativePointer(const RelativePointer& other) noexcept = default;
        RelativePointer(RelativePointer&& other) noexcept;
        RelativePointer& operator=(const RelativePointer& other) noexcept = default;
        RelativePointer& operator=(RelativePointer&& other) noexcept = default;
        
        /// @brief 获取原生指针（假设基地址为 nullptr，直接返回偏移量作为绝对地址）
        ptr_t get() const noexcept;
        
        /// @brief 计算ptr对base的偏移
        static offset_t getOffset(const pool_id_t pool_id, ptr_t const ptr) noexcept;
        
private:
    pool_id_t m_pool_id{0};
    offset_t m_offset{0};
};

} // namespace ZeroCP


#include "../deital/relative_pointer.inl"

#endif