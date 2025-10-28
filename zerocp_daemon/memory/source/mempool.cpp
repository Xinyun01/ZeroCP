#include "mempool.hpp"
#include "relative_pointer.hpp"
#include "mpmclockfreelist.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"

namespace ZeroCP
{
namespace Memory
{

bool MemPool::initialize(   void* baseAddress,
                            void* rawMemory, 
                            uint64_t chunkSize, 
                            uint32_t chunkNums, 
                            void* freeListMemory,
                            uint64_t pool_id) noexcept
{
    if (rawMemory == nullptr || freeListMemory == nullptr)
    {
        ZEROCP_LOG(Error, "MemPool::initialize - Invalid memory address");
        return false;
    }
    if (chunkSize == 0 || chunkNums == 0)
    {
        ZEROCP_LOG(Error, "MemPool::initialize - Invalid parameters: chunkSize={}, chunkNums={}", chunkSize, chunkNums);
        return false;
    }
    m_pool_id = pool_id;
    
    m_chunkSize = chunkSize;
    m_chunkNums = chunkNums;
    // 将 void* 转换为 uint32_t* 用于空闲索引链表
    uint32_t* indicesArray = static_cast<uint32_t*>(freeListMemory);
    m_rawmemory = RelativePointer<void>(indicesArray, rawMemory,uint64_t pool_id);
    m_freeIndices.Initialize(indicesArray, chunkNums); 
    return true;
}

} // namespace Memory
} // namespace ZeroCP

