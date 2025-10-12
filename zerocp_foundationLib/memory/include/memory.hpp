#ifndef MENORY_HPP
#define MENORY_HPP

namespace ZeroCP
{

namespace Memory
{
 
template<typename T>
/**
 * @brief 向上对齐到指定字节数的倍数
 * 
 * @tparam T 传入的整数类型（通常为size_t, uint32_t等）
 * @param value 需要对齐的值
 * @param alignment 对齐的字节数，必须为2的幂
 * @return T 对齐后的结果
 * 
 * 例如: align(13, 8) == 16
 */
T align(const T value, const T alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}
/**
 * @brief 对齐分配内存
 * 
 * @param alignment 对齐字节数，必须为2的幂
 * @param size 需要分配的内存大小
 * @return void* 分配后的内存指针，若分配失败则为nullptr
 * 
 * 此函数用于按照指定字节对齐分配内存。
 */
void* alignedAlloc(const size_t alignment, const size_t size) noexcept;

/**
 * @brief 释放通过 iox::alignedAlloc 分配的内存
 * @details 只应用于 alignedAlloc 返回的指针。不会检查非法指针，释放后内存变为不可用。
 * @param memory  需要释放的对齐内存指针
 */
 void alignedFree(void* const memory) noexcept;

}

}





#endif