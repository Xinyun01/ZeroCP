#include "memory.hpp"
#include <cstdlib>
namespace ZeroCP
{
namespace Memory
{

/**
 * @brief 按照指定对齐字节分配内存
 * 
 * @param alignment 内存对齐字节数，必须是2的幂
 * @param size 分配的字节数
 * @return void* 对齐后的内存指针，分配失败返回nullptr
 * 
 * 实现原理：
 * 1. 多分配 (alignment - 1 + sizeof(void*)) 字节，保证有足够空间实现对齐，并在前面保存原始指针
 * 2. 计算第一个 alignment 对齐的位置，跳过 sizeof(void*) 字节用于存储原始指针
 * 3. 在对齐地址前存储原始 malloc 地址，以供释放
 * 4. 返回对齐后的指针
 */
void* alignedAlloc(const size_t alignment, const size_t size) noexcept
{
    // 申请多余的内存：用户需要的大小 + 对齐修正 + 额外存放原指针的位置
    auto memory = std::malloc(size + alignment - 1 + sizeof(void*));
    if(memory == nullptr)
    {
        return nullptr;  // 分配失败，直接返回
    }

    // 计算对齐后的位置，跳过sizeof(void*)，保证可存储原始指针
    size_t offset = align(reinterpret_cast<size_t>(memory) + sizeof(void*), alignment);
    assert(offset >= reinterpret_cast<size_t>(memory) + sizeof(void*)); // 确保没溢出

    // 在对齐内存前一个位置存下原始malloc返回地址，后续释放时可找回
    reinterpret_cast<void**>(offset)[-1] = memory;

    // 返回对齐后的指针
    return reinterpret_cast<void*>(offset);
}

 void alignedFree(void* const memory) noexcept
 {
    if(memory != nullptr)
    {
        std::free(reinterpret_cast<void**>(memory)[-1]);
    }
 }


}
}