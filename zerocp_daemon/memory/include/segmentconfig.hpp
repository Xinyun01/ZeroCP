#ifndef ZEROCP_SEGMENTCONFIG_HPP
#define ZEROCP_SEGMENTCONFIG_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <map>

namespace ZeroCP
{
namespace Daemon
{

/// @brief 内存段条目 - 描述单个共享内存段的配置
struct SegmentEntry
{
    uint64_t segment_id = 0;                    // 段ID
    std::map<uint64_t, uint64_t> memory_pools;  // 内存池配置: {块大小 -> 块数量}
    std::string m_readerGroup;                  // 读者组名称
    std::string m_writerGroup;                  // 写者组名称 
    /// @brief 验证配置是否有效
    bool isValid() const noexcept;
};

/// @brief 段配置 - 包含所有段的配置信息
struct SegmentConfig
{
    std::vector<SegmentEntry> segment_entries;  // 所有段的配置列表
    
    /// @brief 查找指定ID的段配置
    const SegmentEntry* findSegment(uint64_t segment_id) const noexcept;
    
    /// @brief 获取默认配置（1个段）
    static SegmentConfig getDefaultConfig() noexcept;
};

} // namespace Daemon
} // namespace ZeroCP

#endif // ZEROCP_SEGMENTCONFIG_HPP
