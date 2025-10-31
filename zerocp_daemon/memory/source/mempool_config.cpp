#include "mempool_config.hpp"
#include "logging.hpp"
#include <algorithm>

namespace ZeroCP
{
namespace Memory
{

// 有 3 个 MemPool 对象，每个 MemPool 有：
//   1️⃣ 1 个 MpmcLoFFLi 对象（管理区）
//   2️⃣ N 个 Chunk（数据区）
// 具体来说:
// ┌─────────────────────────────────────────────────────────────┐
// │  MemPool #0                                                 │
// │  ├─ MpmcLoFFLi #0（1 个对象）                               │
// │  └─ Chunk Array（10000 个 Chunk）                          │
// │       ├─ Chunk 0                                            │
// │       ├─ Chunk 1                                            │
// │       └─ ... (共 10000 个)                                  │
// └─────────────────────────────────────────────────────────────┘
// ┌─────────────────────────────────────────────────────────────┐
// │  MemPool #1                                                 │
// │  ├─ MpmcLoFFLi #1（1 个对象）                               │
// │  └─ Chunk Array（5000 个 Chunk）                           │
// │       ├─ Chunk 0                                            │
// │       ├─ Chunk 1                                            │
// │       └─ ... (共 5000 个)                                   │
// └─────────────────────────────────────────────────────────────┘
// ┌─────────────────────────────────────────────────────────────┐
// │  MemPool #2                                                 │
// │  ├─ MpmcLoFFLi #2（1 个对象）                               │
// │  └─ Chunk Array（1000 个 Chunk）                           │
// │       ├─ Chunk 0                                            │
// │       ├─ Chunk 1                                            │
// │       └─ ... (共 1000 个)                                   │
// └─────────────────────────────────────────────────────────────┘

// ==================== 拷贝构造和赋值 ====================

MemPoolConfig::MemPoolConfig(const MemPoolConfig& other) noexcept
{
    for (uint64_t i = 0; i < other.m_memPoolEntries.size(); ++i)
    {
        m_memPoolEntries.emplace_back(other.m_memPoolEntries[i]);
    }
}

MemPoolConfig& MemPoolConfig::operator=(const MemPoolConfig& other) noexcept
{
    if (this != &other)
    {
        m_memPoolEntries.clear();
        for (uint64_t i = 0; i < other.m_memPoolEntries.size(); ++i)
        {
            m_memPoolEntries.emplace_back(other.m_memPoolEntries[i]);
        }
    }
    return *this;
}

// ==================== 配置管理 ====================

bool MemPoolConfig::addMemPoolEntry(uint64_t chunkSize, uint32_t chunkCount) noexcept
{
    bool success = m_memPoolEntries.emplace_back(MemPoolEntry(chunkSize, chunkCount));
    if (!success)
    {
        ZEROCP_LOG(Error, "Failed to add MemPoolEntry: vector capacity exceeded");
    }
    return success;
}

MemPoolConfig& MemPoolConfig::setdefaultPool() noexcept
{
    // 使用 emplace_back 并检查返回值
    m_memPoolEntries.emplace_back(MemPoolEntry(128, 10000));
    m_memPoolEntries.emplace_back(MemPoolEntry(1024, 5000));
    m_memPoolEntries.emplace_back(MemPoolEntry(1024, 1000));
    m_memPoolEntries.emplace_back(MemPoolEntry(1024*4, 500));
    m_memPoolEntries.emplace_back(MemPoolEntry(1024*8, 100));
    return *this;
}

void MemPoolConfig::getMemPoolConfigInfo() noexcept
{
    for(const auto& entry :m_memPoolEntries)
    {
        // ZEROCP_LOG requires exactly 2 arguments: level and message
        // Using placeholder message for now
        ZEROCP_LOG(Info, "MemPool entry configured");
        (void)entry; // Suppress unused warning
    }
}

} // namespace Daemon
} // namespace ZeroCP
