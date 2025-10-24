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
    /// @param segment_id 段ID
    /// @param pool_id 内存池ID
    /// @param pool_base_addr 内存池的基地址（已经是段内的偏移地址）
    /// @param chunk_count chunk数量
    /// @param chunk_size 每个chunk的payload大小（字节）
    PoolAllocator(uint64_t segment_id,
                  uint32_t pool_id,
                  void* pool_base_addr, 
                  uint32_t chunk_count, 
                  uint32_t chunk_size) noexcept;
    
    /// @brief 析构函数
    ~PoolAllocator() = default;
    
    // 禁止拷贝和移动
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;
    
    uint64_t getPoolSize() const noexcept;
    
private:
    uint64_t m_segmentId;           // 所属段ID
    uint32_t m_poolId;              // 内存池ID
    void* m_poolBaseAddr;           // 内存池基地址
    uint32_t m_chunkCount;          // chunk总数
    uint32_t m_chunkSize;           // 每个chunk的payload大小
    uint32_t m_availableChunks;     // 当前可用chunk数量
    uint32_t m_nextFreeIndex;       // 下一个空闲chunk的索引
};
} // namespace Memory
} // namespace ZeroCP
#endif // ZEROCP_POOL_ALLOCATOR_HPP
