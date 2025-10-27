#ifndef ZEROCP_MEMORYPOOL_HPP
#define ZEROCP_MEMORYPOOL_HPP

namespace ZeroCP
{
class Concurrent::MPMC_LockFree_List;
namespace Memory
{
//复用 即使管理chunk块的内存池 同时也是管理chunmanager对象的内存池
class MemPool
{

    RelativePointer<void> m_rawmemory;
    uint64_t m_chunkSize{0};
    uint32_t m_chunkNums{0};
    std::atomic<uint32_t> m_usedChunk{0};
    Concurrent::MpmcLoFFList<uint32_t> m_freeIndices;
}

}

}

#endif // ZEROCP_MEMORYPOOL_HPP