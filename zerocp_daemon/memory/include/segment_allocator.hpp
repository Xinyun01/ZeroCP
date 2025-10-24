#ifndef ZEROCP_SEGMENT_ALLOCATOR_HPP
#define ZEROCP_SEGMENT_ALLOCATOR_HPP

#include <cstdint>
#include <map>
#include <vector>
#include <memory>
#include "pool_allocator.hpp"
#include "segmentconfig.hpp"
#include "posixshm_provider.hpp"

namespace ZeroCP
{
namespace Memory
{

/// @brief 段分配器：管理共享内存段及其内部的内存池
/// @note 负责：
///       1. 根据配置计算每个段所需的总内存大小
///       2. 通过 PosixShmProvider 创建共享内存并获取基地址
///       3. 在段内顺序创建多个 PoolAllocator
class SegmentAllocator
{
public:
    /// @brief 构造函数
    /// @param segmentConfig 给出段配置，进行段分配
    SegmentAllocator(const Daemon::SegmentConfig& segmentConfig) noexcept;
    
    /// @brief 析构函数
    ~SegmentAllocator() = default;
    
    // 禁止拷贝和移动
    SegmentAllocator(const SegmentAllocator&) = delete;
    SegmentAllocator(SegmentAllocator&&) = delete;
    SegmentAllocator& operator=(const SegmentAllocator&) = delete;
    SegmentAllocator& operator=(SegmentAllocator&&) = delete;

    /// @brief 分配所有配置的段
    /// @note 为每个段：计算总大小 -> 创建共享内存 -> 创建内存池
    void allocateSegments() noexcept;
    
private:
    /// @brief 计算单个段所需的总内存大小
    /// @param segmentEntry 段配置条目
    /// @return 总内存大小（字节）
    uint64_t calculateSegmentSize(const Daemon::SegmentEntry& segmentEntry) const noexcept;
    
    Daemon::SegmentConfig m_segmentConfig;  // 段配置
    
    // 每个段的共享内存提供者
    std::map<uint64_t, std::unique_ptr<PosixShmProvider>> m_shmProviders;
    
    // 每个段内的所有内存池分配器
    std::map<uint64_t, std::vector<std::unique_ptr<PoolAllocator>>> m_poolAllocators;
};

} // namespace Memory
} // namespace ZeroCP

#endif // ZEROCP_SEGMENT_ALLOCATOR_HPP
