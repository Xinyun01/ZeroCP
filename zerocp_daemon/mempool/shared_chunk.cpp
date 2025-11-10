#include "shared_chunk.hpp"
#include "zerocp_daemon/memory/include/mempool_manager.hpp"
#include "zerocp_daemon/memory/include/chunk_header.hpp"
#include "zerocp_log/zerocp_log.hpp"

namespace ZeroCP
{
namespace Memory
{

// ==================== 构造函数和析构函数 ====================

SharedChunk::SharedChunk() noexcept
    : m_chunkManager(nullptr)
    , m_memPoolManager(nullptr)
{
}

SharedChunk::SharedChunk(ChunkManager* chunkManager, MemPoolManager* memPoolManager) noexcept
    : m_chunkManager(chunkManager)
    , m_memPoolManager(memPoolManager)
{
    // 注意：这里不增加引用计数！
    // 假设传入的 chunkManager 是刚从 MemPoolManager::getChunk() 返回的
    // 它已经有了正确的引用计数（通常是 1）
    // 这相当于接管所有权，类似于 std::unique_ptr 的构造
}

SharedChunk::SharedChunk(const SharedChunk& other) noexcept
    : m_chunkManager(other.m_chunkManager)
    , m_memPoolManager(other.m_memPoolManager)
{
    // 拷贝构造：增加引用计数
    addRef();
}

SharedChunk& SharedChunk::operator=(const SharedChunk& other) noexcept
{
    if (this != &other)
    {
        // 先释放当前持有的资源
        release();
        
        // 拷贝新资源
        m_chunkManager = other.m_chunkManager;
        m_memPoolManager = other.m_memPoolManager;
        
        // 增加新资源的引用计数
        addRef();
    }
    return *this;
}

SharedChunk::SharedChunk(SharedChunk&& other) noexcept
    : m_chunkManager(other.m_chunkManager)
    , m_memPoolManager(other.m_memPoolManager)
{
    // 移动构造：转移所有权，不增加引用计数
    other.m_chunkManager = nullptr;
    other.m_memPoolManager = nullptr;
}

SharedChunk& SharedChunk::operator=(SharedChunk&& other) noexcept
{
    if (this != &other)
    {
        // 先释放当前持有的资源
        release();
        
        // 转移所有权
        m_chunkManager = other.m_chunkManager;
        m_memPoolManager = other.m_memPoolManager;
        
        // 清空源对象
        other.m_chunkManager = nullptr;
        other.m_memPoolManager = nullptr;
    }
    return *this;
}

SharedChunk::~SharedChunk() noexcept
{
    // 析构：减少引用计数，可能释放资源
    release();
}

// ==================== 公共接口 ====================

uint64_t SharedChunk::useCount() const noexcept
{
    if (m_chunkManager == nullptr)
    {
        return 0;
    }
    return m_chunkManager->m_refCount.load(std::memory_order_acquire);
}

void* SharedChunk::getData() const noexcept
{
    if (m_chunkManager == nullptr)
    {
        return nullptr;
    }
    
    // 通过 RelativePointer 获取 ChunkHeader
    ChunkHeader* header = m_chunkManager->m_chunkHeader.get();
    if (header == nullptr)
    {
        return nullptr;
    }
    
    // 用户数据位于 ChunkHeader 之后的偏移位置
    char* headerAddr = reinterpret_cast<char*>(header);
    return headerAddr + header->m_userPayloadOffset;
}

uint64_t SharedChunk::getSize() const noexcept
{
    if (m_chunkManager == nullptr)
    {
        return 0;
    }
    
    // 通过 RelativePointer 获取 ChunkHeader
    ChunkHeader* header = m_chunkManager->m_chunkHeader.get();
    if (header == nullptr)
    {
        return 0;
    }
    
    // 返回用户数据的大小
    return header->m_userPayloadSize;
}

uint32_t SharedChunk::getChunkManagerIndex() const noexcept
{
    if (m_chunkManager == nullptr)
    {
        return UINT32_MAX;  // 无效索引
    }
    return m_chunkManager->m_chunkManagerIndex;
}

uint32_t SharedChunk::prepareForTransfer() noexcept
{
    if (m_chunkManager == nullptr)
    {
        ZEROCP_LOG(Error, "SharedChunk::prepareForTransfer() - Cannot transfer null chunk");
        return UINT32_MAX;
    }
    
    // 增加引用计数（为目标进程持有的引用）
    uint64_t oldCount = m_chunkManager->m_refCount.fetch_add(1, std::memory_order_acq_rel);
    
    ZEROCP_LOG(Debug, "SharedChunk::prepareForTransfer() - ChunkManager[" 
               << m_chunkManager->m_chunkManagerIndex 
               << "] refCount: " << oldCount << " -> " << (oldCount + 1)
               << " (preparing for cross-process transfer)");
    
    // 返回索引供 IPC 传输使用
    return m_chunkManager->m_chunkManagerIndex;
}

SharedChunk SharedChunk::fromIndex(uint32_t index, MemPoolManager* memPoolManager) noexcept
{
    if (memPoolManager == nullptr)
    {
        ZEROCP_LOG(Error, "SharedChunk::fromIndex() - MemPoolManager is null");
        return SharedChunk();
    }
    
    // 通过索引重建 ChunkManager 指针
    ChunkManager* chunkManager = memPoolManager->getChunkManagerByIndex(index);
    if (chunkManager == nullptr)
    {
        ZEROCP_LOG(Error, "SharedChunk::fromIndex() - Failed to get ChunkManager at index " << index);
        return SharedChunk();
    }
    
    ZEROCP_LOG(Debug, "SharedChunk::fromIndex() - Reconstructed ChunkManager[" << index 
               << "] with refCount=" << chunkManager->m_refCount.load(std::memory_order_acquire));
    
    // 创建 SharedChunk（不增加引用计数，因为发送端已经增加过了）
    return SharedChunk(chunkManager, memPoolManager);
}

void SharedChunk::reset() noexcept
{
    release();
    m_chunkManager = nullptr;
    m_memPoolManager = nullptr;
}

void SharedChunk::reset(ChunkManager* chunkManager, MemPoolManager* memPoolManager) noexcept
{
    // 先释放当前持有的资源
    release();
    
    // 接管新的 ChunkManager（不增加引用计数）
    m_chunkManager = chunkManager;
    m_memPoolManager = memPoolManager;
}

// ==================== 私有方法 ====================

void SharedChunk::addRef() noexcept
{
    if (m_chunkManager != nullptr)
    {
        // 原子性地增加引用计数
        uint64_t oldCount = m_chunkManager->m_refCount.fetch_add(1, std::memory_order_acq_rel);
        
        ZEROCP_LOG(Debug, "SharedChunk::addRef() - ChunkManager[" 
                   << m_chunkManager->m_chunkManagerIndex 
                   << "] refCount: " << oldCount << " -> " << (oldCount + 1));
    }
}

void SharedChunk::release() noexcept
{
    if (m_chunkManager != nullptr && m_memPoolManager != nullptr)
    {
        // 调用 MemPoolManager 的 releaseChunk 方法
        // 它会自动处理引用计数的减少和可能的资源释放
        if (!m_memPoolManager->releaseChunk(m_chunkManager))
        {
            ZEROCP_LOG(Error, "SharedChunk::release() - Failed to release ChunkManager["
                       << m_chunkManager->m_chunkManagerIndex << "]");
        }
        
        // 释放后清空指针
        m_chunkManager = nullptr;
        m_memPoolManager = nullptr;
    }
}

} // namespace Memory
} // namespace ZeroCP

