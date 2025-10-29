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
    /// @brief 构造函数：接受配置引用
    /// @param config 内存池配置的唯一实例引用
    explicit MemPoolAllocator(const MemPoolConfig& config) noexcept;
    
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
    
    /// @brief 创建共享内存并返回映射的基地址
    /// @param name 共享内存名称
    /// @param accessMode 访问模式
    /// @param openMode 打开模式
    /// @param permissions 权限设置
    /// @return 成功返回基地址，失败返回 nullptr
    void* createSharedMemory(const Name_t& name,
                            const AccessMode accessMode,
                            const OpenMode openMode,
                            const Perms permissions) noexcept;
    
    /// @brief 布局内存
    /// @param baseAddress 共享内存基地址
    /// @param memorySize 共享内存总大小
    /// @param targetPools 目标内存池向量（mempools 或 chunkManagerPool）
    /// @return 成功返回 true，失败返回 false
    bool layoutMemory(void* baseAddress, uint64_t memorySize, vector<MemPool, 16>& targetPools) noexcept;
    
private:
    /// @brief 内存池配置引用
    const MemPoolConfig& m_config;
    
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