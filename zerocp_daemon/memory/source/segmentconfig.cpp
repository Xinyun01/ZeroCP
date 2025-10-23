#include "segmentconfig.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <algorithm>

namespace ZeroCP
{
namespace Daemon
{

// ==================== SegmentEntry 实现 ====================
bool SegmentEntry::isValid() const noexcept
{
    // 检查是否为空
    if (memory_pools.empty())
    {
        ZEROCP_LOG(Error, "Invalid SegmentEntry: memory_pools is empty");
        return false;
    }
    
    // 检查所有大小和数量是否为正数
    for (const auto& [pool_size, pool_count] : memory_pools)
    {
        if (pool_size == 0)
        {
            ZEROCP_LOG(Error, "Invalid SegmentEntry: pool_size is zero");
            return false;
        }
        if (pool_count == 0)
        {
            ZEROCP_LOG(Error, "Invalid SegmentEntry: pool_count is zero for size=" << pool_size);
            return false;
        }
    }
    
    return true;
}

// ==================== SegmentConfig 实现 ====================

const SegmentEntry* SegmentConfig::findSegment(uint64_t segment_id) const noexcept
{
    auto it = std::find_if(segment_entries.begin(), segment_entries.end(),
                          [segment_id](const SegmentEntry& entry) {
                              return entry.segment_id == segment_id;
                          });
    
    return (it != segment_entries.end()) ? &(*it) : nullptr;
}

SegmentConfig SegmentConfig::getDefaultConfig() noexcept
{
    SegmentConfig config;
    
    // 创建默认段配置
    SegmentEntry defaultEntry;
    defaultEntry.segment_id = 1;
    defaultEntry.m_writerGroup = "publisher";
    defaultEntry.m_readerGroup = "subscriber";
    
    // 默认内存池配置
    defaultEntry.memory_pools = {
        {128,   10000},  // 128 字节块，10000 个
        {1024,  5000},   // 1KB 块，5000 个
        {4096,  1000}    // 4KB 块，1000 个
    };
    
    config.segment_entries.push_back(defaultEntry);
        
    return config;
}

} // namespace Daemon
} // namespace ZeroCP
