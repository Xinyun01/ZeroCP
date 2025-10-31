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
RelativePointer<T>::RelativePointer(RelativePointer&& other) noexcept
    : m_offset(other.m_offset)
    , m_pool_id(other.m_pool_id)
{
    other.m_offset = 0;
    other.m_pool_id = 0;
}

} // namespace ZeroCP

#endif // ZEROCP_RELATIVE_POINTER_INL

