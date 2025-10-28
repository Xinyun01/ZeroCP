#ifndef BUMP_ALLOCATOR_HPP
#define BUMP_ALLOCATOR_HPP

#include <cstdint>
#include <expected>

namespace ZeroCP
{
namespace Memory
{

//改分配函数主要是为了分配池大小，需要池的地址和计算大小

// BumpAllocator 的错误类型
enum class BumpAllocatorError : uint8_t
{   
        OUT_OF_MEMORY,                // 内存不足
        REQUESTED_ZERO_SIZED_MEMORY   // 请求了零字节的分配
};

/**
 * @brief 简单的bump指针分配器
 *        从预先分配的内存块中连续分配，无法单独释放，仅支持整体重置
 */
class BumpAllocator
{
public:
    /**
     * @brief 构造函数，指定起始内存地址和总长度
     * @param startAddress 内存块起始地址
     * @param length 内存字节数
     */
    BumpAllocator(void* const startAddress, const uint64_t length) noexcept;

    // 不可拷贝，只允许移动
    BumpAllocator(const BumpAllocator&) = delete;
    BumpAllocator(BumpAllocator&&) noexcept = default;
    BumpAllocator& operator=(const BumpAllocator&) = delete;
    BumpAllocator& operator=(BumpAllocator&&) noexcept = default;
    /**
     * @brief 析构函数
     */
    ~BumpAllocator();

    /**
     * @brief 分配指定字节数且满足alignment对齐的内存
     * @param size 分配字节数
     * @param alignment 对齐字节数（2的幂）
     * @return std::expected<void*, BumpAllocatorError> 分配成功则为指针，失败返回错误码
     */
    std::expected<void*, BumpAllocatorError> allocate(const uint64_t size, const uint64_t alignment) noexcept;

private:
    uint64_t const m_startAddress{0U}; // 内存块起始地址
    uint64_t m_length{0U};             // 内存总长度
    uint64_t m_currentAddressLength{0U};//当前分配器已经分配的长度
    uint64_t m_alignment{8U};          // 对齐字节数（2的幂）
};

}
}


#endif