#ifndef ZEROCP_MEMORYPOOL_HPP
#define ZEROCP_MEMORYPOOL_HPP

#include <cstdint>
#include <atomic>
#include "relative_pointer.hpp"
#include "mpmclockfreelist.hpp"

namespace ZeroCP
{
namespace Memory
{

//复用 即使管理chunk块的内存池 同时也是管理chunmanager对象的内存池
class MemPool
{
public:
    /// @brief 带参数的构造函数 - 直接在构造时完成所有初始化
    /// @param baseAddress 共享内存基地址
    /// @param rawMemory 原始内存地址
    /// @param chunkSize 单个 chunk 的大小
    /// @param chunkNums chunk 的数量
    /// @param freeListMemory 空闲索引链表的内存地址
    /// @param pool_id 内存池ID
    /// @note 这个构造函数允许 emplace_back 直接在 vector 内部构造完全初始化的 MemPool 对象
    /// @note 对象构造完成后立即可用，遵循 RAII 原则
    MemPool(void* baseAddress, 
            void* rawMemory, 
            uint64_t chunkSize, 
            uint32_t chunkNums, 
            void* freeListMemory,
            uint64_t pool_id) noexcept;
    
    // 删除默认构造（强制使用带参数的构造函数）
    MemPool() = delete;
    
    // 删除复制和移动（包含 std::atomic 和 MPMC_LockFree_List 不可复制/移动）
    MemPool(const MemPool&) = delete;
    MemPool(MemPool&&) = delete;
    MemPool& operator=(const MemPool&) = delete;
    MemPool& operator=(MemPool&&) = delete;
    
    /// @brief 获取 chunk 大小
    uint64_t getChunkSize() const noexcept { return m_chunkSize; }
    
    /// @brief 获取 chunk 总数
    uint32_t getTotalChunks() const noexcept { return m_chunkNums; }
    
    /// @brief 获取已使用的 chunk 数量
    uint32_t getUsedChunks() const noexcept { return m_usedChunk.load(std::memory_order_relaxed); }
    
    /// @brief 获取空闲 chunk 数量
    uint32_t getFreeChunks() const noexcept { return m_chunkNums - getUsedChunks(); }
    
    /// @brief 设置原始内存地址（用于延迟初始化）
    /// @param rawMemory 原始内存地址
    /// @param dataOffset 数据区偏移量（相对于数据区基地址）
    void setRawMemory(void* rawMemory, uint64_t dataOffset) noexcept;
    
    /// @brief 获取数据区偏移量
    /// @return 相对于数据区基地址的偏移量
    uint64_t getDataOffset() const noexcept { return m_dataOffset; }

private:
    ZeroCP::RelativePointer<void> m_rawMemory;      ///< 数据池的基地址相对指针
    uint64_t m_chunkSize{0};                        ///< 当前的池的chunk大小
    uint32_t m_chunkNums{0};                        ///< 当前的池的chunk数量
    std::atomic<uint32_t> m_usedChunk{0};           ///< 当前的池的已使用chunk数量
    uint64_t m_pool_id{0};                          ///< 当前的池的id
    uint64_t m_dataOffset{0};                       ///< 池首偏移（相对于数据区基地址），用于多进程通信
    ZeroCP::Concurrent::MPMC_LockFree_List m_freeIndices; ///< 当前的池的空闲chunk索引链表
};

}

}

#endif // ZEROCP_MEMORYPOOL_HPP