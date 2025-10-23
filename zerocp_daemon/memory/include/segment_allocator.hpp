#ifndef ZEROCP_SEGMENT_ALLOCATOR_HPP
#define ZEROCP_SEGMENT_ALLOCATOR_HPP

#include <cstdint>
#include <map>
#include <vector>
#include <memory>
#include "pool_allocator.hpp"

namespace ZeroCP
{
namespace Daemon
{

/// @brief 内存池元数据 - 描述单个内存池的布局和状态
/// @note 存储在共享内存中，使用偏移而非指针
struct PoolMetadata
{
    uint64_t pool_size;        // 每个块的大小（字节，已对齐）
    uint64_t pool_count;       // 块总数
    uint64_t pool_offset;      // 内存池起始偏移（相对于段基地址）
    uint64_t allocated_count;  // 已分配块数
    
    /// @brief 获取总大小
    uint64_t getTotalSize() const noexcept {
        return pool_size * pool_count;
    }
    
    /// @brief 检查是否已满
    bool isFull() const noexcept {
        return allocated_count >= pool_count;
    }
    
    /// @brief 获取使用率百分比
    double getUsagePercent() const noexcept {
        return pool_count > 0 ? (allocated_count * 100.0 / pool_count) : 0.0;
    }
};

/// @brief 段头部 - 存储在共享内存段开头的元数据
/// @note 这个结构体会被直接映射到共享内存的开头
///       在跨进程通信中，每个进程都能通过相同的共享内存名称映射到各自的虚拟地址
///       但通过相对偏移，所有进程都能正确访问同一块数据
struct SegmentHeader
{
    static constexpr uint32_t MAGIC_NUMBER = 0x5A45524F;  // "ZERO"
    static constexpr uint32_t VERSION = 1;
    static constexpr uint64_t ALIGNMENT = 64;  // 64字节对齐（缓存行）
    
    uint32_t magic;              // 魔数，用于验证
    uint32_t version;            // 版本号
    uint64_t segment_id;         // 段ID
    uint64_t total_size;         // 段总大小
    uint32_t pool_count;         // 内存池数量
    uint32_t reserved;           // 预留对齐
    
    // 变长数组：PoolMetadata pool_metadata[pool_count]
    // 紧接着这个结构体存储
    
    /// @brief 验证段头部是否有效
    bool isValid() const noexcept {
        return magic == MAGIC_NUMBER && version == VERSION;
    }
    
    /// @brief 获取池元数据数组的偏移
    static constexpr uint64_t getPoolMetadataOffset() noexcept {
        return sizeof(SegmentHeader);
    }
    
    /// @brief 获取池元数据数组的起始地址（本进程）
    PoolMetadata* getPoolMetadataArray() noexcept {
        return reinterpret_cast<PoolMetadata*>(
            reinterpret_cast<uint8_t*>(this) + sizeof(SegmentHeader)
        );
    }
    
    const PoolMetadata* getPoolMetadataArray() const noexcept {
        return reinterpret_cast<const PoolMetadata*>(
            reinterpret_cast<const uint8_t*>(this) + sizeof(SegmentHeader)
        );
    }
    
    /// @brief 计算段头部总大小（包括所有池元数据）
    static uint64_t calculateHeaderSize(uint32_t pool_count) noexcept {
        uint64_t size = sizeof(SegmentHeader) + sizeof(PoolMetadata) * pool_count;
        // 对齐到ALIGNMENT
        return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    }
};

/// @brief 段池分配器 - 管理一个段内的所有内存池
/// @note 一个段对应一个完整的POSIX共享内存映射
///       段内包含多个内存池，每个池有独立的起始偏移地址
///       支持O(1)快速定位：段ID → 共享内存基地址 → 池偏移 → 具体块
///       
///       跨进程通信模型：
///       进程A映射: 共享内存 -> 虚拟地址 0x7f0000000000
///       进程B映射: 共享内存 -> 虚拟地址 0x7f8000000000
///       数据偏移:  offset = 0x1000 (相对于段基地址)
///       进程A访问: 0x7f0000000000 + 0x1000
///       进程B访问: 0x7f8000000000 + 0x1000
///       都能访问到同一块共享内存数据！
class SegmentAllocator
{
public:
    /// @brief 构造函数
    /// @param segment_id 段ID
    /// @param base_address 段起始地址（共享内存在本进程的映射地址）
    /// @param total_size 段总大小
    /// @param pools 内存池配置 {块大小 -> 块数量}
    SegmentAllocator(uint64_t segment_id,
                     void* base_address,
                     uint64_t total_size,
                     const std::map<uint64_t, uint64_t>& pools) noexcept;
    
    /// @brief 析构函数
    ~SegmentAllocator() = default;
    
    // 禁止拷贝和移动
    SegmentAllocator(const SegmentAllocator&) = delete;
    SegmentAllocator(SegmentAllocator&&) = delete;
    SegmentAllocator& operator=(const SegmentAllocator&) = delete;
    SegmentAllocator& operator=(SegmentAllocator&&) = delete;
    
    /// @brief 分配内存（返回偏移，支持跨进程）
    /// @param size 请求的大小
    /// @return 分配的内存偏移（相对于段基地址），0表示分配失败
    /// @note 会选择最小的能容纳size的池进行分配（Best-Fit策略）
    uint64_t allocate(uint64_t size) noexcept;
    
    /// @brief 释放内存（使用偏移）
    /// @param offset 要释放的内存偏移（相对于段基地址）
    /// @return true if successful, false otherwise
    bool deallocate(uint64_t offset) noexcept;
    
    /// @brief 偏移转指针（用于本进程访问共享内存）
    /// @param offset 相对偏移
    /// @return 本进程中的虚拟地址
    void* offsetToPointer(uint64_t offset) const noexcept;
    
    /// @brief 指针转偏移（用于跨进程传递）
    /// @param ptr 本进程中的虚拟地址
    /// @return 相对偏移（可跨进程使用）
    uint64_t pointerToOffset(void* ptr) const noexcept;
    
    /// @brief 获取段ID
    uint64_t getSegmentId() const noexcept { return m_segmentId; }
    
    /// @brief 获取基地址（本进程的映射地址）
    void* getBaseAddress() const noexcept { return m_baseAddress; }
    
    /// @brief 获取段头部
    SegmentHeader* getHeader() noexcept { return m_header; }
    const SegmentHeader* getHeader() const noexcept { return m_header; }
    
    /// @brief 获取指定大小池的分配器
    PoolAllocator* getPoolAllocator(uint64_t pool_size) noexcept;
    
    /// @brief 获取所有池的统计信息
    std::map<uint64_t, PoolMetadata> getPoolStats() const noexcept;
    
    /// @brief 打印段的统计信息
    void printStats() const noexcept;
    
    /// @brief 检查初始化是否成功
    bool isInitialized() const noexcept { return m_initialized; }
    
private:
    uint64_t m_segmentId;                                  // 段ID
    void* m_baseAddress;                                   // 段起始地址（本进程映射）
    uint64_t m_totalSize;                                  // 段总大小
    SegmentHeader* m_header;                               // 段头部
    bool m_initialized;                                    // 初始化标志
    std::map<uint64_t, std::unique_ptr<PoolAllocator>> m_poolAllocators;  // 池分配器映射 {块大小 -> 分配器}
    
    /// @brief 初始化段头部和内存池
    /// @param pools 内存池配置 {块大小 -> 块数量}
    /// @return true if successful, false otherwise
    bool initialize(const std::map<uint64_t, uint64_t>& pools) noexcept;
    
    /// @brief 找到能容纳指定大小的最小池（Best-Fit）
    PoolAllocator* findBestFitPool(uint64_t size) noexcept;
};

} // namespace Daemon
} // namespace ZeroCP

#endif // ZEROCP_SEGMENT_ALLOCATOR_HPP
