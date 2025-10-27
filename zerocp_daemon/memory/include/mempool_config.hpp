#ifndef ZEROCP_MEMPOOL_CONFIG_HPP
#define ZEROCP_MEMPOOL_CONFIG_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <map>

namespace ZeroCP
{
namespace Memory
{

//段配置，主要是对内存池的配置提供内存池大小等信息 提供查找内存池的信息
struct MemPoolConfig
{
    struct MemPoolEntry noexcept
    {
    MemPoolEntry(uint64_t poolSize, uint32_t poolCount) 
    : m_poolSize(poolSize), m_poolCount(poolCount)
    {}
    uint64_t m_poolSize {0};
    uint32_t m_poolCount{0};
    };
    std::vector<MemPoolEntry> m_memPoolEntries;  // 所有段的配置列表

    /// @brief 默认构造函数
    MemPoolConfig() noexcept = default;

    /// @brief 禁用拷贝构造
    MemPoolConfig(const MemPoolConfig&) noexcept = delete;

    /// @brief 允许移动构造
    MemPoolConfig(MemPoolConfig&&) noexcept = default;

    /// @brief 禁用拷贝赋值
    MemPoolConfig& operator=(const MemPoolConfig&) noexcept = delete;

    /// @brief 允许移动赋值
    MemPoolConfig& operator=(MemPoolConfig&&) noexcept = default;

    /// @brief 析构函数
    ~MemPoolConfig() noexcept = default;

    /// @brief 添加内存池配置项
    void addMemPoolEntry(uint64_t poolSize, uint32_t poolCount) noexcept;

    /// @brief 获取内存池配置信息
    void getMemPoolConfigInfo() noexcept;

    /// @brief 设置默认内存池配置
    MemPoolConfig& setdefaultPool() noexcept;
};


} // namespace Daemon
} // namespace ZeroCP
#endif //   
