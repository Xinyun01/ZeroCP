#ifndef ZEROCP_CHUNKMANAGER_HPP
#define ZEROCP_CHUNKMANAGER_HPP

#include <atomic>
#include "relative_pointer.hpp"

namespace ZeroCP
{
namespace Memory
{
class MemPool;
class ChunkHeader;

//ChunkManager 主要用来原理chunk的具体数据，传输只传出ChunkManager的相对偏移在通过RelativePointer转换 
struct ChunkManager
{
    using base_t = ChunkHeader;
    ZeroCP::RelativePointer<base_t> m_chunkHeader;
    /// @brief 指向数据所属内存池的相对指针
    ZeroCP::RelativePointer<MemPool> m_mempool;
    /// @brief 指向ChunkManagement对象池的相对指针
    ZeroCP::RelativePointer<MemPool> m_chunkManagementPool;
    std::atomic<uint64_t> m_refCount{0};
    
    // ==================== 跨进程索引信息 ====================
    /// @brief 数据 chunk 在其所属池中的索引（用于跨进程地址重建）
    uint32_t m_chunkIndex{0};
    /// @brief ChunkManager 对象在 ChunkManagerPool 中的索引
    uint32_t m_chunkManagerIndex{0};
};

} // namespace Memory
} // namespace ZeroCP

#endif // ZEROCP_CHUNKMANAGER_HPP