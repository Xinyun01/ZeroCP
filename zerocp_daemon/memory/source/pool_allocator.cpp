#include "pool_allocator.hpp"
#include "zerocp_foundationLib/memory/include/memory.hpp"
#include "zerocp_foundationLib/memory/include/bump_allocator.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "chunkheader.hpp"
#include <cstring>
#include <algorithm>

namespace ZeroCP
{
namespace Memory
{

/**
 * @brief 构造函数：初始化内存池分配器
 * @param segment_id 段ID
 * @param pool_id 内存池ID
 * @param pool_base_addr 内存池的基地址（已经是段内的偏移地址）
 * @param chunk_count chunk数量
 * @param chunk_size 每个chunk的payload大小（字节）
 * 
 * @note 构造函数中使用 BumpAllocator 在内存池中初始化所有 chunk
 *       每个 chunk 包含：ChunkHeader + UserPayload
 */
PoolAllocator::PoolAllocator(void* segment_base, 
                            uint64_t block_size, 
                            uint64_t block_count) noexcept
    : m_segmentBase(segment_base)
    , m_blockSize(block_size)  // TODO: 需要实现 alignBlockSize 函数
    , m_blockCount(block_count)
{
}

uint64_t PoolAllocator::getPoolSize() const noexcept
{
    //数据段的大小
    auto memoryPoolSize = ZeroCP::Memory::align(m_blockSize,8U) * m_blockCount;
    //数据头部大小
    auto headerSize = ZeroCP::Memory::align(sizeof(ChunkHeader),8U)*m_blockCount;
    //管理断大小
    auto manageSize = ZeroCP::Concurrent::MPMC_LockFree_List::requiredIndexMemorySize(m_blockCount);
    return memoryPoolSize + headerSize + manageSize;
}

} // namespace Memory
} // namespace ZeroCP