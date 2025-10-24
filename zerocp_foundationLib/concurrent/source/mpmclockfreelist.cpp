#include "mpmclockfreelist.hpp"
namespace ZeroCP
{
namespace Concurrent
{

// 构造函数，初始化空闲节点索引头指针和容量
MPMC_LockFree_List::MPMC_LockFree_List(uint32_t* freeIndicesHeader, uint32_t capacity) noexcept
    : m_freeIndicesHeader(freeIndicesHeader)
    , m_capacity(capacity)
{

}

// 初始化链表，将所有节点串成空闲链表，并设置无效节点索引
void MPMC_LockFree_List::Initialize()
{
    m_invalidNodeIndex = m_capacity; // 无效节点索引，通常设置为容量（capacity）
    for(uint32_t i = 0; i < m_capacity; ++i)
    {
        m_freeIndicesHeader[i] = i+1; // 下一个节点索引，最后一个暂时设置
    }
    m_freeIndicesHeader[m_capacity-1] = m_invalidNodeIndex; // 最后一个节点指向无效索引
}

bool MPMC_LockFree_List::pop(uint32_t& nodeIndex) const noexcept
{
    
    
    if(headNode.nextNodeIndex == m_invalidNodeIndex)
    {
        return false; // 链表为空
    }
    nodeIndex = headNode.nextNodeIndex; // 获取头节点索引
    
    if(m_headIndex.compare_exchange_strong(headNode, Node{headNode.nextNodeIndex, headNode.abaCounts+1}))
    {
        return true;
    }
    return false;
}

bool MPMC_LockFree_List::push(const uint32_t nodeIndex) noexcept
{
    std::atomic_thread_fence(std::memory_order_acquire);

    if(m_freeIndicesHeader[nodeIndex] != m_invalidNodeIndex || nodeIndex >= m_capacity)
}

uint64_t MPMC_LockFree_List::requiredIndexMemorySize(const uint32_t capacity) const noexcept
{
    retunr ZeroCP::Memory::align(sizeof(Index_t),8U)*(capacity+1);
}

}
}