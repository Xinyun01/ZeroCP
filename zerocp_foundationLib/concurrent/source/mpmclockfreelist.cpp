#include "mpmclockfreelist.hpp"

namespace ZeroCP
{
namespace Concurrent
{

// 构造函数，初始化空闲节点索引头指针和容量
MPMC_LockFree_List::MPMC_LockFree_List(uint32_t* freeIndicesHeader, uint32_t capacity) noexcept
    : m_headIndex(Node{0U, 1U})
    , m_freeIndicesHeader(freeIndicesHeader)
    , m_capacity(capacity)
{
}

// 初始化链表，将所有节点串成空闲链表，并设置无效节点索引
void MPMC_LockFree_List::Initialize()
{
    m_invalidNodeIndex = m_capacity; // 无效节点索引，通常设置为容量（capacity）
    for(uint32_t i = 0; i < m_capacity; ++i)
    {
        m_freeIndicesHeader[i] = i + 1; // 下一个节点索引
    }
    m_freeIndicesHeader[m_capacity - 1] = m_invalidNodeIndex; // 最后一个节点指向无效索引
}

// 出栈操作，弹出链表头节点
bool MPMC_LockFree_List::pop(uint32_t& nodeIndex) noexcept
{
    Node headNode = m_headIndex.load(std::memory_order_acquire);
    
    while(true)
    {
        // 检查链表是否为空
        if(headNode.nextNodeIndex == m_invalidNodeIndex)
        {
            return false; // 链表为空
        }
        
        // 获取下一个节点的索引
        uint32_t nextNodeIndex = m_freeIndicesHeader[headNode.nextNodeIndex];
        
        // 尝试更新头节点，增加ABA计数
        Node newHead{nextNodeIndex, headNode.abaCounts + 1};
        
        if(m_headIndex.compare_exchange_weak(headNode, newHead, 
                                             std::memory_order_release,
                                             std::memory_order_acquire))
        {
            nodeIndex = headNode.nextNodeIndex; // 返回弹出的节点索引
            return true;
        }
        // CAS 失败，重新加载头节点继续尝试
    }
}

// 入栈操作，将节点推入链表头部
bool MPMC_LockFree_List::push(const uint32_t nodeIndex) noexcept
{
    // 检查节点索引有效性
    if(nodeIndex >= m_capacity)
    {
        return false; // 节点索引越界
    }
    
    Node headNode = m_headIndex.load(std::memory_order_acquire);
    
    while(true)
    {
        // 将当前头节点设置为新节点的下一个节点
        m_freeIndicesHeader[nodeIndex] = headNode.nextNodeIndex;
        
        // 尝试更新头节点，增加ABA计数
        Node newHead{nodeIndex, headNode.abaCounts + 1};
        
        if(m_headIndex.compare_exchange_weak(headNode, newHead,
                                             std::memory_order_release,
                                             std::memory_order_acquire))
        {
            return true;
        }
        // CAS 失败，重新加载头节点继续尝试
    }
}

uint64_t MPMC_LockFree_List::requiredIndexMemorySize(const uint32_t capacity) noexcept
{
    return (capacity + 1) * sizeof(uint32_t);
}

uint64_t MPMC_LockFree_List::getNodeSize() const noexcept
{
    return sizeof(uint32_t);
}

} // namespace Concurrent
} // namespace ZeroCP
