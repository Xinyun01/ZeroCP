#ifndef ZEROCP_POOL_ALLOCATOR_HPP 
#define ZEROCP_POOL_ALLOCATOR_HPP 

#include <cstdint>

namespace ZeroCP
{
namespace Memory
{

/// @brief 单个内存池分配器
/// @note 当前的类主要是接受段分配器的内存池参数，进行内存对齐分配
///       使用相对偏移地址而非绝对指针，支持跨进程共享内存寻址
///       同时这个内存申请是连续内存申请，在寻址可以进行O(1)寻址，提高性能
class PoolAllocator
{
public:
    /// @brief 构造函数
    /// @param segment_base 段基地址（用于偏移计算）
    /// @param pool_offset 内存池起始偏移（相对于段基地址）
    /// @param block_size 块大小（字节）
    /// @param block_count 块数量
    PoolAllocator(  void* segment_base, 
                    uint64_t block_size, 
                    uint64_t block_count ) noexcept;
    
    /// @brief 析构函数
    ~PoolAllocator() = default;
    
    // 禁止拷贝和移动
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;
    
    /// @brief 分配一个块（O(1)时间复杂度）
    /// @return 分配的内存偏移（相对于段基地址），0表示分配失败
    uint64_t memoryPoolSize() noexcept;
    uint64_t getBlockOffset(uint64_t index) const noexcept;
    
    void allocateMemoryPool() const noexcept;
private:
    const void* m_segmentBase;           // 段基地址（本进程的映射地址）
    uint64_t m_currentAddress{0U};         // 当前分配地址
    uint64_t m_blockSize;          // 块大小（已对齐）
    uint64_t m_blockCount;         // 块总数
    /// @brief 获取块的偏移（O(1)索引计算）

    
};

} // namespace Memory
} // namespace ZeroCP

#endif // ZEROCP_POOL_ALLOCATOR_HPP
