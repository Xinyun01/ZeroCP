#ifndef ZEROCP_SHARED_CHUNK_HPP
#define ZEROCP_SHARED_CHUNK_HPP

#include <cstdint>
#include <atomic>
#include "zerocp_daemon/memory/include/chunk_manager.hpp"

namespace ZeroCP
{
namespace Memory
{

class MemPoolManager;

/// @brief SharedChunk - ChunkManager 的 RAII 包装器
/// @details 管理 ChunkManager 的引用计数，类似于 shared_ptr 的语义
/// - 构造时：增加引用计数
/// - 拷贝时：增加引用计数
/// - 移动时：转移所有权，不增加引用计数
/// - 析构时：减少引用计数，当引用计数为 0 时释放资源
/// 适用于多进程环境，ChunkManager 存储在共享内存中
class SharedChunk
{
public:
    /// @brief 默认构造函数 - 创建空的 SharedChunk
    SharedChunk() noexcept;
    
    /// @brief 构造函数 - 接管一个已存在的 ChunkManager（不增加引用计数）
    /// @param chunkManager 指向共享内存中的 ChunkManager 对象
    /// @param memPoolManager 用于释放资源的 MemPoolManager 指针
    /// @note 假设传入的 chunkManager 已经有正确的引用计数（通常是刚分配的，refCount=1）
    explicit SharedChunk(ChunkManager* chunkManager, MemPoolManager* memPoolManager) noexcept;
    
    /// @brief 拷贝构造函数 - 增加引用计数
    SharedChunk(const SharedChunk& other) noexcept;
    
    /// @brief 拷贝赋值运算符 - 增加引用计数
    SharedChunk& operator=(const SharedChunk& other) noexcept;
    
    /// @brief 移动构造函数 - 转移所有权，不增加引用计数
    SharedChunk(SharedChunk&& other) noexcept;
    
    /// @brief 移动赋值运算符 - 转移所有权，不增加引用计数
    SharedChunk& operator=(SharedChunk&& other) noexcept;
    
    /// @brief 析构函数 - 减少引用计数，可能释放资源
    ~SharedChunk() noexcept;
    
    // ==================== 访问接口 ====================
    
    /// @brief 获取底层的 ChunkManager 指针
    ChunkManager* get() const noexcept { return m_chunkManager; }
    
    /// @brief 检查是否持有有效的 ChunkManager
    bool isValid() const noexcept { return m_chunkManager != nullptr; }
    
    /// @brief 转换为 bool，用于条件判断
    explicit operator bool() const noexcept { return isValid(); }
    
    /// @brief 获取当前引用计数
    uint64_t useCount() const noexcept;
    
    /// @brief 获取 chunk 数据的指针（通过 ChunkHeader）
    void* getChunkheader() const noexcept;
    
    /// @brief 获取 chunk 的大小
    uint64_t getSize() const noexcept;
    void* getUserPayload() const noexcept;
    
    /// @brief 获取 ChunkManager 的索引（用于跨进程传输）
    /// @return ChunkManager 在 ChunkManagerPool 中的索引
    /// @note 这个索引可以安全地通过 IPC 传输到其他进程
    uint32_t getChunkManagerIndex() const noexcept;
    
    /// @brief 准备跨进程传输：增加引用计数并返回索引
    /// @return ChunkManager 的索引，可以通过 IPC 传输
    /// @note 调用此方法后，目标进程应该使用 fromIndex() 重建 SharedChunk
    /// @warning 如果目标进程最终没有接收，会导致内存泄漏！
    uint32_t prepareForTransfer() noexcept;
    
    /// @brief 从索引重建 SharedChunk（接收端使用）
    /// @param index ChunkManager 的索引（从 prepareForTransfer 获取）
    /// @param memPoolManager MemPoolManager 实例
    /// @return 重建的 SharedChunk（不增加引用计数）
    /// @note 假设发送端已经调用了 prepareForTransfer() 增加引用计数
    static SharedChunk fromIndex(uint32_t index, MemPoolManager* memPoolManager) noexcept;
    
    /// @brief 释放当前持有的 ChunkManager（减少引用计数）
    void reset() noexcept;
    
    /// @brief 替换为新的 ChunkManager
    void reset(ChunkManager* chunkManager, MemPoolManager* memPoolManager) noexcept;

private:
    /// @brief 增加引用计数（用于拷贝）
    void addRef() noexcept;
    
    /// @brief 减少引用计数并可能释放资源（用于析构和赋值）
    void release() noexcept;

private:
    /// @brief 指向共享内存中的 ChunkManager
    ChunkManager* m_chunkManager{nullptr};
};

} // namespace Memory
} // namespace ZeroCP

#endif // ZEROCP_SHARED_CHUNK_HPP