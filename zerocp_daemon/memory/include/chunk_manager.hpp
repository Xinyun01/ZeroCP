#ifndef ZEROCP_CHUNKMANAGER_HPP
#define ZEROCP_CHUNKMANAGER_HPP

namespace ZeroCP
{
class MemPool;
class ChunkHeader;

namespace Memory
{
//ChunkManager 主要用来原理chunk的具体数据，传输只传出ChunkManager的相对偏移在通过RelativePointer转换 
struct ChunkManager
{
    using base_t = ChunkHeader;
    iox::RelativePointer<base_t> m_chunkHeader;
    /// @brief 指向数据所属内存池的相对指针
    iox::RelativePointer<MemPool> m_mempool;
    /// @brief 指向ChunkManagement对象池的相对指针
    iox::RelativePointer<MemPool> m_chunkManagementPool;
    std::atomic<uint64_t> m_refCount{0};
}
}

}

#endif // ZEROCP_CHUNKMANAGER_HPP