#include "mempool.hpp"
#include "relative_pointer.hpp"
#include "mpmclockfreelist.hpp"
#include "logging.hpp"

namespace ZeroCP
{
namespace Memory
{

// 带参数的构造函数实现
MemPool::MemPool(void* baseAddress,
                 void* rawMemory,
                 uint64_t chunkSize,
                 uint32_t chunkNums,
                 void* freeListMemory,
                 uint64_t pool_id) noexcept
    : m_rawMemory(baseAddress, rawMemory, pool_id)  // 初始化 RelativePointer
    , m_chunkSize(chunkSize)
    , m_chunkNums(chunkNums)
    , m_usedChunk(0)  // 初始化 atomic
    , m_pool_id(pool_id)
    , m_freeIndices(static_cast<uint32_t*>(freeListMemory), chunkNums)  // 初始化 MPMC_LockFree_List
{
    // 验证参数
    if (rawMemory == nullptr || freeListMemory == nullptr)
    {
        ZEROCP_LOG(Error, "MemPool constructor - Invalid memory address");
        // 注意：构造函数无法返回错误，这里只能记录日志
        // 实际应用中可能需要添加一个 isValid() 方法来检查
        return;
    }
    if (chunkSize == 0 || chunkNums == 0)
    {
        ZEROCP_LOG(Error, "MemPool constructor - Invalid parameters");
        return;
    }
    
    // 初始化 MPMC_LockFree_List 的内部状态
    m_freeIndices.Initialize();
    
    ZEROCP_LOG(Info, "MemPool constructed successfully - ChunkSize: " << chunkSize 
               << ", ChunkNums: " << chunkNums << ", PoolID: " << pool_id);
}

void MemPool::setRawMemory(void* rawMemory, uint64_t dataOffset) noexcept
{
    if (rawMemory == nullptr)
    {
        ZEROCP_LOG(Error, "setRawMemory - Invalid memory address");
        return;
    }
    
    // 注意：RelativePointer 已经在构造函数中初始化，这里只更新 dataOffset
    // m_rawMemory 已经在构造函数中设置，指向正确的 rawMemory
    // 这个方法现在主要用于更新 dataOffset
    
    // 记录数据区偏移量（用于多进程通信）
    m_dataOffset = dataOffset;
    
    ZEROCP_LOG(Info, "MemPool dataOffset updated: " << dataOffset);
}

} // namespace Memory
} // namespace ZeroCP

