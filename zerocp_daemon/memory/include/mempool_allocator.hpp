#ifndef ZEROCP_MEMPOOL_ALLOCATOR_HPP
#define ZEROCP_MEMPOOL_ALLOCATOR_HPP

#include "mempool_config.hpp"
#include "mempool_manager.hpp"
#include "posixshm_provider.hpp"
#include <memory>
#include <expected>
#include <cstdint>
#include "mempool_allocator.hpp"
namespace ZeroCP
{
namespace Memory
{

/// @brief 内存池分配器：整合配置、内存计算和共享内存创建
/// @details 职责：
///   1. 接收配置，计算所需共享内存大小
///   2. 创建共享内存（通过 PosixShmProvider）
///   3. 获取共享内存基地址，用于后续内存池初始化
class MemPoolAllocator
{
public:
    /// @brief 构造函数：接受配置引用和共享内存基地址
    /// @param config 内存池配置的唯一实例引用
    /// @param sharedMemoryBase 共享内存基地址（用于 RelativePointer）
    explicit MemPoolAllocator(const MemPoolConfig& config, void* sharedMemoryBase) noexcept;
    
    /// @brief 禁用默认构造
    MemPoolAllocator() noexcept = delete;
    
    /// @brief 禁用拷贝构造
    MemPoolAllocator(const MemPoolAllocator&) noexcept = delete;
    
    /// @brief 允许移动构造
    MemPoolAllocator(MemPoolAllocator&&) noexcept = default;
    
    /// @brief 禁用拷贝赋值
    MemPoolAllocator& operator=(const MemPoolAllocator&) noexcept = delete;
    
    /// @brief 允许移动赋值
    MemPoolAllocator& operator=(MemPoolAllocator&&) noexcept = default;
    
    /// @brief 析构函数
    ~MemPoolAllocator() noexcept = default;
    

    
    /// @brief 布局管理内存区：在共享内存中创建 vector 和 MemPool 对象
    /// @param mgmtBaseAddress 管理内存区基地址
    /// @param mgmtMemorySize 管理内存区大小
    /// @param mempools 返回指向共享内存中创建的 MemPool vector 的指针
    /// @param chunkManagerPool 返回指向共享内存中创建的 ChunkManager pool 的指针
    /// @return 成功返回 true，失败返回 false
    bool ManagementMemoryLayout(void* mgmtBaseAddress, uint64_t mgmtMemorySize,
                                vector<MemPool, 16>& mempools,
                                vector<MemPool, 1>& chunkManagerPool) noexcept;
    
    /// @brief 布局数据内存区（分配 chunk 块并设置到 MemPool）
    /// @param baseAddress 数据内存区基地址
    /// @param memorySize 数据内存区大小
    /// @param mempools 共享内存中的 MemPool vector 引用
    /// @return 成功返回 true，失败返回 false
    bool ChunkMemoryLayout(void* baseAddress, uint64_t memorySize,
                          vector<MemPool, 16>& mempools) noexcept;
    
private:
    /// @brief 内存池配置引用
    const MemPoolConfig& m_config;
    
    /// @brief 共享内存基地址（用于 RelativePointer）
    void* m_sharedMemoryBase{nullptr};
    
    /// @brief 共享内存提供者：管理共享内存生命周期
    std::unique_ptr<PosixShmProvider> m_shmProvider;
    
    /// @brief 计算得到的总内存大小
    uint64_t m_totalMemorySize{0};
    
    /// @brief 共享内存基地址
    void* m_baseAddress{nullptr};
};

} // namespace Memory
} // namespace ZeroCP

#endif // ZEROCP_MEMPOOL_ALLOCATOR_HPP