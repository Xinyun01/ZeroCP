#ifndef ZEROCP_FOUNDATIONLIB_STATICSTL_VECTOR_HPP
#define ZEROCP_FOUNDATIONLIB_STATICSTL_VECTOR_HPP

//本文件主要是为了重写vector 
// 在编译期固定容量，
// 在栈或共享内存中存储，
// 无动态内存分配（不使用 new/malloc），
// 接口尽量与 std::vector 类似

namespace ZeroCP
{

template<typename T, size_t Capacity>
class Static_Vector{

    using value_type = T
    using iterator = T*;

    //
    Static_Vector() noexcept = default;
    Static_Vector(const uint64_t count, const T& value) noexcept;
    ~Static_Vector();
}
}

#include "zerocp/foundationLib/staticstl/detail/vector.inl"

#endif