#include "segment_allocator.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "zerocp_foundationLib/memory/include/memory.hpp"
#include "zerocp_foundationLib/filesystem/include/filesystem.hpp"
#include "chunkheader.hpp"
#include <cstring>
#include <algorithm>

namespace ZeroCP
{
namespace Memory
{

/**
 * @brief 构造函数：初始化段分配器
 * @param segmentConfig 段配置信息（包含所有段的配置列表）
 */
SegmentAllocator::SegmentAllocator(const Daemon::SegmentConfig& segmentConfig) noexcept
    : m_segmentConfig(segmentConfig)
{
}

/**
 * @brief 计算单个段所需的总内存大小
 * @param segmentEntry 段配置条目
 * @return 总内存大小（字节）
 * 
 * @note 计算公式：
 *       段总大小 = Σ(每个内存池的大小)
 *       每个内存池大小 = chunk数量 × (chunk_payload + ChunkHeader + 对齐填充)
 */
uint64_t SegmentAllocator::calculateSegmentSize(const Daemon::SegmentEntry& segmentEntry) const noexcept
{
    uint64_t totalSize = 0;
    
    for (const auto& poolConfig : segmentEntry.memory_pools)
    {
        // 计算单个 chunk 的总大小
        // chunk 总大小 = ChunkHeader + userPayload + 对齐填充
        // poolConfig: {chunk_size, chunk_count}
        uint64_t chunkSize = poolConfig.first;   // chunk payload 大小
        uint64_t chunkCount = poolConfig.second;  // chunk 数量
        
        // 创建临时的 PoolAllocator 对象来计算内存池大小
        // 注意：这里只是为了计算大小，所以传入临时参数
        auto poolAllocator = std::make_unique<PoolAllocator>(
            segmentEntry.segment_id,  // segment_id
            0,                        // pool_id (临时值)
            nullptr,                  // pool_base_addr (临时值)
            static_cast<uint32_t>(chunkCount),
            static_cast<uint32_t>(chunkSize)
        );
        
        // 累加每个内存池的大小
        totalSize += poolAllocator->getPoolSize();
    }
      
    return totalSize;
}

/**
 * @brief 分配所有配置的段
 * 
 * @note 流程：
 *       1. 遍历每个段配置
 *       2. 计算段的总大小（所有内存池大小之和）
 *       3. 通过 PosixShmProvider 创建共享内存并获取基地址
 *       4. 在段内按顺序创建每个内存池，传入基地址和偏移量
 */
void SegmentAllocator::allocateSegments() noexcept
{
   
        //step1；计算段的大小
        auto segmentTotalSize = calculateSegmentSize(m_segmentConfig.segment_entries[0]);
        // Step 2: 创建共享内存并获取基地址
        // 构造 PosixShmProvider：需要提供 name, size, accessMode, openMode, permissions
        auto shmProvider = std::make_unique<PosixShmProvider>(
            segmentEntry.segment_id,                    // name (segment_id 作为名称)
            segmentTotalSize,                           // memorySize
            AccessMode::ReadWrite,                      // accessMode
            OpenMode::CreateOrOpen,                     // openMode
            Perms::OwnerAll                            // permissions
        );
        
        // 创建共享内存
        auto createResult = shmProvider->createMemory();
        if (!createResult.has_value())
        {
            ZEROCP_LOG(Error, "Failed to create shared memory for segment");
            return;
        }
        
        void* segmentBaseAddr = shmProvider->getBaseAddress();
        
        if (segmentBaseAddr == nullptr)
        {
            ZEROCP_LOG(Error, "Failed to get base address for segment");
            continue;
        }
        
        ZEROCP_LOG(Info, "Segment {} shared memory created at {}, size={}", segmentEntry.segment_id, segmentBaseAddr, segmentTotalSize);
        
        // Step 3: 在段内按顺序创建每个内存池
        uint64_t currentOffset = 0;
        std::vector<std::unique_ptr<PoolAllocator>> poolsInSegment;
        
        for (const auto& poolConfig : segmentEntry.memory_pools)
        {
            // 计算当前内存池的大小
            uint64_t chunkHeaderSize = sizeof(ChunkHeader);
            uint64_t userPayloadSize = poolConfig.chunk_size;
            uint64_t alignment = std::max(poolConfig.chunk_size, static_cast<uint32_t>(alignof(ChunkHeader)));
            uint64_t chunkTotalSize = ZeroCP::Memory::align(chunkHeaderSize + userPayloadSize, alignment);
            uint64_t poolSize = poolConfig.chunk_count * chunkTotalSize;
            
            // 计算当前内存池的基地址（段基地址 + 偏移量）
            void* poolBaseAddr = static_cast<char*>(segmentBaseAddr) + currentOffset;     
            // Step 4: 创建 PoolAllocator，传入基地址和配置
            auto poolAllocator = std::make_unique<PoolAllocator>(
                segmentEntry.segment_id,
                poolConfig.pool_id,
                poolBaseAddr,
                poolConfig.chunk_count,
                poolConfig.chunk_size
            );
            
            poolsInSegment.push_back(std::move(poolAllocator));
            
            // 更新偏移量，指向下一个内存池的起始位置
            currentOffset += poolSize;
        }
        
        // 保存段的共享内存提供者和内存池分配器
        m_shmProviders[segmentEntry.segment_id] = std::move(shmProvider);
        m_poolAllocators[segmentEntry.segment_id] = std::move(poolsInSegment);
        
    
    ZEROCP_LOG(Info, "All segments allocated successfully");
}

} // namespace Memory
} // namespace ZeroCP

