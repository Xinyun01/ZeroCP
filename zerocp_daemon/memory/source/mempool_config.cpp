#include "segmentconfig.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
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

void MemPoolConfig::addMemPoolEntry(uint64_t poolSize, uint32_t poolCount) noexcept
{
    m_memPoolEntries.push_back(MemPoolEntry(poolSize, poolCount));
}

MemPoolConfig& MemPoolConfig::setdefaultPool() noexcept
{
    m_memPoolEntries.push_back(MemPoolEntry(128, 10000));
    m_memPoolEntries.push_back(MemPoolEntry(1024, 5000));
    m_memPoolEntries.push_back(MemPoolEntry(1024, 1000));
    m_memPoolEntries.push_back(MemPoolEntry(1024*4, 500));
    m_memPoolEntries.push_back(MemPoolEntry(1024*8, 100));
    return *this;
}

void MemPoolConfig::getMemPoolConfigInfo() noexcept
{
    for(const auto& entry :m_memPoolEntries)
    {
        ZEROCP_LOG(Info) << "Pool Size: " << entry.m_poolSize << ", Pool Count: " << entry.m_poolCount;
    }
}

} // namespace Daemon
} // namespace ZeroCP
