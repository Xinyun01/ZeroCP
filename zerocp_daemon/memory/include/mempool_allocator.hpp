#ifndef ZEROCP_MEMPOOL_ALLOCATOR_HPP
#define ZEROCP_MEMPOOL_ALLOCATOR_HPP

#include "mempool_config.hpp"
#include "mempool_manager.hpp"
#include "posixshm_provider.hpp"
#include <memory>
#include <expected>
#include <cstdint>

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

    /// @brief 初始化共享内存：计算大小并创建
    /// @param shmName 共享内存名称
    /// @return 成功返回基地址，失败返回错误
    std::expected<void*, Details::PosixSharedMemoryObjectError> 
    initializeSharedMemory(const std::string& shmName) noexcept;
    
    /// @brief 初始化内存池：按照配置布局并初始化所有内存池
    /// @details 调用 MemPoolManager::initialize() 在共享内存中创建所有内存池
    /// @return 成功返回 true，失败返回 false
    /// @note 必须在 initializeSharedMemory() 成功后调用
    bool initializeMemPools() noexcept;
    
    /// @brief 获取共享内存基地址
    /// @return 共享内存基地址，未初始化则返回 nullptr
    void* getBaseAddress() const noexcept;
    
    /// @brief 获取共享内存总大小
    /// @return 共享内存总大小（字节）
    uint64_t getMemorySize() const noexcept;
    
    /// @brief 检查是否已初始化
    /// @return true 表示共享内存和内存池都已初始化
    bool isInitialized() const noexcept;
    
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