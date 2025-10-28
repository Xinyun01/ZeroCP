#ifndef ZEROCP_MEMORYPOOL_HPP
#define ZEROCP_MEMORYPOOL_HPP

#include <cstdint>
#include <atomic>

namespace ZeroCP
{
namespace Memory
{

class Concurrent::MPMC_LockFree_List;

//复用 即使管理chunk块的内存池 同时也是管理chunmanager对象的内存池
class MemPool
{
public:
    /// @brief 初始化内存池
    /// @param rawMemory 原始内存地址
    /// @param chunkSize 单个 chunk 的大小
    /// @param chunkNums chunk 的数量
    /// @param freeListMemory 空闲索引链表的内存地址
    /// @return 初始化是否成功
    bool initialize(void* rawMemory, uint64_t chunkSize, uint32_t chunkNums, void* freeListMemory,uint64_t pool_id) noexcept;
    
    /// @brief 获取 chunk 大小
    uint64_t getChunkSize() const noexcept { return m_chunkSize; }
    
    /// @brief 获取 chunk 总数
    uint32_t getTotalChunks() const noexcept { return m_chunkNums; }
    
    /// @brief 获取已使用的 chunk 数量
    uint32_t getUsedChunks() const noexcept { return m_usedChunk.load(std::memory_order_relaxed); }
    
    /// @brief 获取空闲 chunk 数量
    uint32_t getFreeChunks() const noexcept { return m_chunkNums - getUsedChunks(); }

private:
    RelativePointer<void> m_rawmemory;
    uint64_t m_chunkSize{0};
    uint32_t m_chunkNums{0};
    std::atomic<uint32_t> m_usedChunk{0};
    uint64_t m_pool_id{0};
    Concurrent::MpmcLoFFList<uint32_t> m_freeIndices;
};

}

}

#endif // ZEROCP_MEMORYPOOL_HPP