#ifndef ZEROCP_MEMPOOL_CONFIG_HPP
#define ZEROCP_MEMPOOL_CONFIG_HPP

#include <cstdint>
#include "vector.hpp"

namespace ZeroCP
{
namespace Memory
{

/// @brief 内存池配置（完全可在共享内存中）
/// @note 使用 ZeroCP::vector 替代 std::vector 以支持共享内存
struct MemPoolConfig
{
    /// @brief 单个内存池的配置项
    struct MemPoolEntry
    {
        MemPoolEntry() noexcept = default;
        
        MemPoolEntry(uint64_t chunkSize, uint32_t chunkCount) noexcept 
            : m_chunkSize(chunkSize), m_chunkCount(chunkCount)
    {}
        
        uint64_t m_chunkSize {0};   ///< 单个 chunk 大小（重命名自 m_poolSize）
        uint32_t m_chunkCount{0};   ///< chunk 数量（重命名自 m_poolCount）
    };
    
    /// @brief 内存池配置列表（最多支持 16 个内存池）
    ZeroCP::vector<MemPoolEntry, 16> m_memPoolEntries;

    /// @brief 默认构造函数
    MemPoolConfig() noexcept = default;

    /// @brief 拷贝构造（允许，用于复制到共享内存）
    MemPoolConfig(const MemPoolConfig& other) noexcept;

    /// @brief 移动构造（允许）
    MemPoolConfig(MemPoolConfig&&) noexcept = default;

    /// @brief 拷贝赋值（允许，用于复制到共享内存）
    MemPoolConfig& operator=(const MemPoolConfig& other) noexcept;

    /// @brief 移动赋值（允许）
    MemPoolConfig& operator=(MemPoolConfig&&) noexcept = default;

    /// @brief 析构函数
    ~MemPoolConfig() noexcept = default;

    /// @brief 添加内存池配置项
    /// @param chunkSize 单个 chunk 的大小（字节）
    /// @param chunkCount chunk 的数量
    /// @return 成功返回 true，失败（超出容量）返回 false
    bool addMemPoolEntry(uint64_t chunkSize, uint32_t chunkCount) noexcept;

    /// @brief 获取内存池配置信息
    void getMemPoolConfigInfo() noexcept;

    /// @brief 设置默认内存池配置
    MemPoolConfig& setdefaultPool() noexcept;
};


} // namespace Daemon
} // namespace ZeroCP
#endif //   
