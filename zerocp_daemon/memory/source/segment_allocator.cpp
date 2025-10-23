#include "segment_allocator.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "zerocp_foundationLib/memory/include/memory.hpp"
#include <cstring>
#include <iostream>

namespace ZeroCP
{
namespace Daemon
{

// ==================== SegmentAllocator 实现 ====================

SegmentAllocator::SegmentAllocator(uint64_t segment_id,
                                   void* base_address,
                                   uint64_t total_size,
                                   const std::map<uint64_t, uint64_t>& pools) noexcept
    : m_segmentId(segment_id)
    , m_baseAddress(base_address)
    , m_totalSize(total_size)
    , m_header(static_cast<SegmentHeader*>(base_address))
    , m_initialized(false)
{
    ZEROCP_LOG(Info, "Creating SegmentAllocator: id=" << segment_id
                     << ", base=" << base_address
                     << ", size=" << total_size);
    
    m_initialized = initialize(pools);
    
    if (m_initialized)
    {
        ZEROCP_LOG(Info, "SegmentAllocator initialized successfully");
    }
    else
    {
        ZEROCP_LOG(Error, "SegmentAllocator initialization failed");
    }
}

bool SegmentAllocator::initialize(const std::map<uint64_t, uint64_t>& pools) noexcept
{
    if (m_baseAddress == nullptr)
    {
        ZEROCP_LOG(Error, "Invalid base address (nullptr)");
        return false;
    }
    
    if (pools.empty())
    {
        ZEROCP_LOG(Error, "No pools configured");
        return false;
    }
    
    // 1. 初始化段头部
    m_header->magic = SegmentHeader::MAGIC_NUMBER;
    m_header->version = SegmentHeader::VERSION;
    m_header->segment_id = m_segmentId;
    m_header->total_size = m_totalSize;
    m_header->pool_count = static_cast<uint32_t>(pools.size());
    m_header->reserved = 0;
    
    // 2. 计算头部大小（包括所有池元数据）
    uint64_t header_size = SegmentHeader::calculateHeaderSize(m_header->pool_count);
    uint64_t current_offset = header_size;  // 第一个池从头部之后开始
    
    ZEROCP_LOG(Debug, "Header size: " << header_size << " bytes, pool count: " << pools.size());
    
    // 3. 获取池元数据数组
    PoolMetadata* pool_metadata = m_header->getPoolMetadataArray();
    
    // 4. 为每个池分配空间并创建分配器
    uint32_t pool_index = 0;
    for (const auto& [block_size, block_count] : pools)
    {
        // 对齐块大小到默认对齐（8字节）
        uint64_t aligned_block_size = Memory::align(block_size, SegmentHeader::ALIGNMENT);
        uint64_t pool_size = aligned_block_size * block_count;
        
        // 对齐池起始偏移
        current_offset = Memory::align(current_offset, SegmentHeader::ALIGNMENT);
        
        // 检查是否超出段大小
        if (current_offset + pool_size > m_totalSize)
        {
            ZEROCP_LOG(Error, "Pool " << pool_index << " exceeds segment size: "
                              << "offset=" << current_offset
                              << ", pool_size=" << pool_size
                              << ", total_size=" << m_totalSize);
            return false;
        }
        
        // 记录池元数据
        pool_metadata[pool_index].pool_size = aligned_block_size;
        pool_metadata[pool_index].pool_count = block_count;
        pool_metadata[pool_index].pool_offset = current_offset;
        pool_metadata[pool_index].allocated_count = 0;
        
        // 创建池分配器
        auto allocator = std::make_unique<PoolAllocator>(
            m_baseAddress,
            current_offset,
            aligned_block_size,
            block_count,
            SegmentHeader::ALIGNMENT
        );
        
        m_poolAllocators[aligned_block_size] = std::move(allocator);
        
        ZEROCP_LOG(Debug, "Pool " << pool_index << " created: "
                          << "block_size=" << block_size << " (aligned=" << aligned_block_size << ")"
                          << ", count=" << block_count
                          << ", offset=" << current_offset
                          << ", pool_size=" << pool_size);
        
        current_offset += pool_size;
        pool_index++;
    }
    
    ZEROCP_LOG(Info, "Total used: " << current_offset << " / " << m_totalSize 
                     << " bytes (" << (current_offset * 100.0 / m_totalSize) << "%)");
    
    return true;
}

uint64_t SegmentAllocator::allocate(uint64_t size) noexcept
{
    if (!m_initialized)
    {
        ZEROCP_LOG(Error, "SegmentAllocator not initialized");
        return 0;
    }
    
    if (size == 0)
    {
        ZEROCP_LOG(Warning, "Requested allocation size is 0");
        return 0;
    }
    
    // 找到最佳匹配的池（Best-Fit）
    PoolAllocator* pool = findBestFitPool(size);
    if (pool == nullptr)
    {
        ZEROCP_LOG(Warning, "No suitable pool found for size " << size);
        return 0;
    }
    
    // 从池中分配
    uint64_t offset = pool->allocate();
    if (offset == 0)
    {
        ZEROCP_LOG(Warning, "Pool allocation failed for size " << size);
        return 0;
    }
    
    // 更新元数据中的已分配计数
    PoolMetadata* pool_metadata = m_header->getPoolMetadataArray();
    for (uint32_t i = 0; i < m_header->pool_count; i++)
    {
        if (pool_metadata[i].pool_offset == pool->getPoolOffset())
        {
            pool_metadata[i].allocated_count = pool->getAllocatedCount();
            break;
        }
    }
    
    ZEROCP_LOG(Debug, "Allocated " << size << " bytes at offset " << offset
                      << " (pool block_size=" << pool->getBlockSize() << ")");
    
    return offset;
}

bool SegmentAllocator::deallocate(uint64_t offset) noexcept
{
    if (!m_initialized)
    {
        ZEROCP_LOG(Error, "SegmentAllocator not initialized");
        return false;
    }
    
    if (offset == 0)
    {
        ZEROCP_LOG(Warning, "Attempted to deallocate offset 0");
        return false;
    }
    
    // 找到拥有此偏移的池
    for (auto& [block_size, pool] : m_poolAllocators)
    {
        if (pool->ownsOffset(offset))
        {
            bool success = pool->deallocate(offset);
            
            if (success)
            {
                // 更新元数据中的已分配计数
                PoolMetadata* pool_metadata = m_header->getPoolMetadataArray();
                for (uint32_t i = 0; i < m_header->pool_count; i++)
                {
                    if (pool_metadata[i].pool_offset == pool->getPoolOffset())
                    {
                        pool_metadata[i].allocated_count = pool->getAllocatedCount();
                        break;
                    }
                }
                
                ZEROCP_LOG(Debug, "Deallocated offset " << offset);
            }
            
            return success;
        }
    }
    
    ZEROCP_LOG(Error, "Offset " << offset << " does not belong to any pool in this segment");
    return false;
}

PoolAllocator* SegmentAllocator::findBestFitPool(uint64_t size) noexcept
{
    PoolAllocator* best_fit = nullptr;
    uint64_t min_block_size = UINT64_MAX;
    
    // 找到能容纳size的最小块
    for (auto& [block_size, pool] : m_poolAllocators)
    {
        if (block_size >= size && !pool->isFull())
        {
            if (block_size < min_block_size)
            {
                min_block_size = block_size;
                best_fit = pool.get();
            }
        }
    }
    
    return best_fit;
}

void* SegmentAllocator::offsetToPointer(uint64_t offset) const noexcept
{
    if (offset == 0)
    {
        return nullptr;
    }
    
    if (offset >= m_totalSize)
    {
        ZEROCP_LOG(Error, "Offset " << offset << " exceeds segment size " << m_totalSize);
        return nullptr;
    }
    
    return static_cast<uint8_t*>(m_baseAddress) + offset;
}

uint64_t SegmentAllocator::pointerToOffset(void* ptr) const noexcept
{
    if (ptr == nullptr)
    {
        return 0;
    }
    
    uint8_t* byte_ptr = static_cast<uint8_t*>(ptr);
    uint8_t* base_ptr = static_cast<uint8_t*>(m_baseAddress);
    
    if (byte_ptr < base_ptr)
    {
        ZEROCP_LOG(Error, "Pointer " << ptr << " is before segment base " << m_baseAddress);
        return 0;
    }
    
    uint64_t offset = byte_ptr - base_ptr;
    
    if (offset >= m_totalSize)
    {
        ZEROCP_LOG(Error, "Pointer " << ptr << " exceeds segment bounds");
        return 0;
    }
    
    return offset;
}

PoolAllocator* SegmentAllocator::getPoolAllocator(uint64_t pool_size) noexcept
{
    auto it = m_poolAllocators.find(pool_size);
    return (it != m_poolAllocators.end()) ? it->second.get() : nullptr;
}

std::map<uint64_t, PoolMetadata> SegmentAllocator::getPoolStats() const noexcept
{
    std::map<uint64_t, PoolMetadata> stats;
    
    const PoolMetadata* pool_metadata = m_header->getPoolMetadataArray();
    for (uint32_t i = 0; i < m_header->pool_count; i++)
    {
        stats[pool_metadata[i].pool_size] = pool_metadata[i];
    }
    
    return stats;
}

void SegmentAllocator::printStats() const noexcept
{
    std::cout << "\n========== Segment Statistics ==========\n";
    std::cout << "Segment ID: " << m_segmentId << "\n";
    std::cout << "Base Address: " << m_baseAddress << "\n";
    std::cout << "Total Size: " << m_totalSize << " bytes\n";
    std::cout << "Pool Count: " << m_header->pool_count << "\n";
    std::cout << "----------------------------------------\n";
    
    const PoolMetadata* pool_metadata = m_header->getPoolMetadataArray();
    for (uint32_t i = 0; i < m_header->pool_count; i++)
    {
        const auto& meta = pool_metadata[i];
        std::cout << "Pool " << i << ":\n";
        std::cout << "  Block Size: " << meta.pool_size << " bytes\n";
        std::cout << "  Block Count: " << meta.pool_count << "\n";
        std::cout << "  Offset: " << meta.pool_offset << "\n";
        std::cout << "  Allocated: " << meta.allocated_count << " / " << meta.pool_count;
        std::cout << " (" << meta.getUsagePercent() << "%)\n";
        std::cout << "  Total Size: " << meta.getTotalSize() << " bytes\n";
    }
    
    std::cout << "========================================\n\n";
}

} // namespace Daemon
} // namespace ZeroCP

