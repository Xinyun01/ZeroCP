#ifndef MENORY_HPP
#define MENORY_HPP

#include <cstddef>
#include <cstdint>

namespace ZeroCP
{

namespace Memory
{

/// @brief 计算对齐后的大小
/// @param size 原始大小
/// @param alignment 对齐字节数，必须是2的幂
/// @return 对齐后的大小
uint64_t align(uint64_t size, uint64_t alignment) noexcept;
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