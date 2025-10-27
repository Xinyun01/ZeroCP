#ifndef ZEROC_MPMCLOCKFREELIST_HPP
#define ZEROC_MPMCLOCKFREELIST_HPP
#include "relative_pointer.hpp"
namespace ZeroCP
{
namespace Concurrent
{
//静态链表
// 多生产者多消费者无锁链表（MPMC Lock-Free List）
class MPMC_LockFree_List
{
    using Index_t = uint32_t;
public:
    // 链表节点结构体
    struct alignas(8) Node
    {
        uint32_t nextNodeIndex; // 下一个节点的索引
        uint32_t abaCounts;     // ABA防护计数
    };

    // 构造函数：无操作，noexcept保证异常安全
    MPMC_LockFree_List(uint32_t* freeIndicesHeader, uint32_t capacity) noexcept = default;
    // 析构函数：无操作，noexcept保证异常安全
    ~MPMC_LockFree_List() noexcept = default;
    // 初始化链表
    void Initialize();
    static uint64_t requiredIndexMemorySize(const uint32_t capacity) const noexcept;
    // 获取节点大小
    uint64_t getNodeSize() const noexcept;

    // 入栈操作，将节点索引推入链表栈顶
    bool push(const uint32_t nodeIndex) noexcept;
    // 出栈操作，将链表栈顶节点弹出
    bool pop(uint32_t& nodeIndex) noexcept;
    
private:

    std::atomic<Node> m_headIndex{0U,1U}; // 头节点索引（含ABA防护计数），使用原子类型以支持无锁并发
    uint32_t m_invalidNodeIndex{0}; // 无效节点索引标记
    ZeroCP::RelativePointer<uint32_t>* m_freeIndicesHeader{nullptr}; // 空闲节点索引头指针
    uint32_t m_capacity{0}; // 链表容量
};


}
}