#ifndef ZEROCP_POOL_REGISTRY_HPP
#define ZEROCP_POOL_REGISTRY_HPP

#include <cstdint>
#include <unordered_map>
#include <mutex>

namespace ZeroCP
{

// 段ID类型定义
using pool_id_t = std::uint64_t;

// 共享内存段注册表：存储每个segment_id对应的基地址
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
        RelativePointer(const RelativePointer& other) noexcept = default;
        RelativePointer(RelativePointer&& other) noexcept;
        /// @brief 计算ptr对base的偏移
        static offset_t getOffset(const segment_id_t id, ptr_t const ptr) noexcept;
        
private:
    pool_id_t m_pool_id{0};
    offset_t m_offset{0};
};

} // namespace ZeroCP


#include "zerocp_foundationLib/posix/memory/deital/relative_pointer.inl"

#endif