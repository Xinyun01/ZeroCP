#ifndef ZEROCP_RELATIVE_POINTER_INL
#define ZEROCP_RELATIVE_POINTER_INL

#include <cassert>
#include "zerocp_foundationLib/report/include/logging.hpp"

namespace ZeroCP
{


// ==================== RelativePointer 实现 ====================
template<typename T>
RelativePointer<T>::RelativePointer(ptr_t baseAddress, ptr_t const ptr, uint64_t pool_id) noexcept
    : m_ptr(getOffset(baseAddress, ptr))
    , m_pool_id(pool_id)
{
}

template<typename T>
RelativePointer<T>::RelativePointer(ptr_t const ptr, uint64_t pool_id) noexcept
    : m_ptr(getOffset(pool_id, ptr))
    , m_pool_id(pool_id)
{
}


template<typename T>
offset_t RelativePointer<T>::get_offset(ptr_t const ptr) noexcept
{
    return reinterpret_cast<offset_t>(ptr) - reinterpret_cast<offset_t>(baseAddress);
}



} // namespace ZeroCP

#endif // ZEROCP_RELATIVE_POINTER_INL

