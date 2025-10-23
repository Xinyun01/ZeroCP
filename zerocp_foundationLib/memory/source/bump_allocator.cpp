#include "bump_allocator.hpp"
#include "memory.hpp"

namespace ZeroCP
{
namespace Memory
{
    
BumpAllocator::BumpAllocator(void* const startAddress, const uint64_t length) noexcept
    : m_startAddress(reinterpret_cast<uint64_t>(startAddress))
    , m_length(length)
{
}

BumpAllocator::~BumpAllocator() = default;

std::expected<void*, BumpAllocatorError> BumpAllocator::allocate(const uint64_t size, const uint64_t alignment) noexcept
{
    // 检查请求的分配大小是否为0
    if (size == 0U)
    {
        // 返回错误：请求了零字节内存
        return std::unexpected(BumpAllocatorError::REQUESTED_ZERO_SIZED_MEMORY);
    }

    // 计算当前实际分配地址
    const uint64_t currentAddress = m_currentAddress + m_startAddress;

    // 对齐当前地址到 alignment 的倍数
    uint64_t alignedPosition = Memory::align(currentAddress, alignment);
    alignedPosition -= m_startAddress; // 换算为相对于起始地址的偏移量

    // 检查对齐后位置加上分配大小是否超出分配器容量
    const uint64_t nextPosition = alignedPosition + size;
    if (nextPosition > m_length)
    {
        // 返回错误：内存不足
        return std::unexpected(BumpAllocatorError::OUT_OF_MEMORY);
    }

    // 获得分配完成后的对齐指针
    void* alignedAddress = reinterpret_cast<void*>(alignedPosition + m_startAddress);

    // 更新 bump 指针为下一个可用地址
    m_currentAddress = nextPosition;

    // 返回成功与分配的内存指针
    return alignedAddress;
}

} // namespace Memory
} // namespace ZeroCP