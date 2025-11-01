#ifndef ZEROCP_RELATIVE_POINTER_INL
#define ZEROCP_RELATIVE_POINTER_INL

#include <cassert>
#include "../../../report/include/logging.hpp"

namespace ZeroCP
{


// ==================== RelativePointer 实现 ====================
template<typename T>
RelativePointer<T>::RelativePointer(ptr_t baseAddress, ptr_t const ptr, uint64_t pool_id) noexcept
    : m_pool_id(pool_id)
    , m_offset(reinterpret_cast<offset_t>(ptr) - reinterpret_cast<offset_t>(baseAddress))
{
}

template<typename T>
RelativePointer<T>::RelativePointer(ptr_t ptr, uint64_t pool_id) noexcept
    : m_pool_id(pool_id)
    , m_offset(reinterpret_cast<offset_t>(ptr))
{
    // 简化构造函数：假设 baseAddress 为 nullptr，直接存储绝对地址作为偏移量
    // 这适用于管理区内存，其中所有进程都映射到相同的虚拟地址
}

template<typename T>
RelativePointer<T>::RelativePointer(RelativePointer&& other) noexcept
    : m_offset(other.m_offset)
    , m_pool_id(other.m_pool_id)
{
    other.m_offset = 0;
    other.m_pool_id = 0;
}

template<typename T>
typename RelativePointer<T>::ptr_t RelativePointer<T>::get() const noexcept
{
    // 简化版本：直接将偏移量作为绝对地址返回
    // 这适用于使用简化构造函数创建的 RelativePointer
    return reinterpret_cast<ptr_t>(m_offset);
}

} // namespace ZeroCP

#endif // ZEROCP_RELATIVE_POINTER_INL

