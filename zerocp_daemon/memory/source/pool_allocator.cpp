#include "pool_allocator.hpp"
#include "memory.hpp"
#include "bump_allocator.hpp"
#include <cstring>
#include "chunkheader.hpp"
#include "logging.hpp"
namespace ZeroCP
{
namespace Memory
{

PoolAllocator::PoolAllocator(void* segment_base, 
                            uint64_t block_size, 
                            uint64_t block_count) noexcept
    : m_segmentBase(segment_base)
    , m_blockSize(alignBlockSize(block_size, alignment))
    , m_blockCount(block_count);



uint64_t PoolAllocator::memoryPoolSize() noexcept
{
    return ( m_blockSize+sizeof(ChunkHeader) )*m_blockCount;
}

void PoolAllocator::allocateMemoryPool() const noexcept
{

    auto memoryPoolSize = memoryPoolSize();
    auto m_bumpAllocator = BumpAllocator(m_segmentBase, memoryPoolSize);
    auto result = m_bumpAllocator.allocate(memoryPoolSize, m_alignment);

    if(result.has_error())
    {
        ZEROCP_LOG(Error, "PoolAllocator::allocateMemoryPool failed");
        return 0U;
    }

    m_currentAddress =static_cast<uint64_t> result.value();
}
} // namespace ZeroCP
} // namespace Memory