#ifndef ZEROCP_MEMPOOL_MANAGER_HPP
#define ZEROCP_MEMPOOL_MANAGER_HPP

#include "vector.hpp"
#include <mutex>
#include <cstdint>

namespace ZeroCP
{
namespace Memory
{

class MemPool;
class MemPoolConfig;
class ChunkManager;

/// @brief 内存池管理器（全局单例）
/// @details 职责：
///   1. 管理所有内存池实例
///   2. 计算所需共享内存大小
///   3. 提供内存分配/释放接口
///   4. 管理 chunk 引用计数
class MemPoolManager
{
public:
    // ==================== 单例模式 ====================
    
    /// @brief 获取全局唯一实例
    /// @param config 内存池配置引用（仅首次调用时使用）
    /// @return 全局 MemPoolManager 实例的引用
    static MemPoolManager& getInstance(const MemPoolConfig& config) noexcept;
    
    /// @brief 获取已初始化的全局实例
    /// @return 全局实例指针，如果未初始化则返回 nullptr
    static MemPoolManager* getInstanceIfInitialized() noexcept;
    
    // 禁用拷贝和移动
    MemPoolManager(const MemPoolManager&) noexcept = delete;
    MemPoolManager(MemPoolManager&&) noexcept = delete;
    MemPoolManager& operator=(const MemPoolManager&) noexcept = delete;
    MemPoolManager& operator=(MemPoolManager&&) noexcept = delete;
    
    ~MemPoolManager() noexcept = default;


    // ==================== 内存大小计算 ====================
    
    /// @brief 计算管理区需要的内存大小
    uint64_t getManagementMemorySize() const noexcept;
    
    /// @brief 计算chunk区需要的内存大小
    uint64_t getChunkMemorySize() const noexcept;
    
    /// @brief 计算总共需要的内存大小
    uint64_t getTotalMemorySize() const noexcept;

    // ==================== 核心分配/释放接口 ====================
    
    /// @brief 分配指定大小的 chunk
    /// @param size 请求的内存大小（字节）
    /// @return 成功返回 ChunkManager 指针，失败返回 nullptr
    /// @note 自动选择最小满足条件的内存池
    ChunkManager* getChunk(uint64_t size) noexcept;
    
    /// @brief 释放 chunk（引用计数减到0时才真正释放）
    /// @param chunkManager chunk 管理器指针
    /// @return 成功返回 true
    bool releaseChunk(ChunkManager* chunkManager) noexcept;
    /// @brief 打印所有内存池状态
    void printAllPoolStats() const noexcept;
    
    // ==================== 访问器接口 ====================
    
    /// @brief 获取内存池列表的引用
    /// @return 内存池列表的引用
    vector<MemPool, 16>& getMemPools() noexcept;
    
    /// @brief 获取 ChunkManager 内存池的引用
    /// @return ChunkManager 内存池的引用
    vector<MemPool, 1>& getChunkManagerPool() noexcept;

private:

    

    // ==================== 成员变量 ====================
    
    const MemPoolConfig& m_config;              ///< 配置引用
    vector<MemPool, 16> m_mempools;             ///< 数据 chunk 池（最多16个）
    vector<MemPool, 1> m_chunkManagerPool;      ///< ChunkManager 对象池
       
    static MemPoolManager* s_instance;           ///< 全局单例实例
    static std::mutex s_mutex;                   ///< 保护单例创建的互斥锁
};

} // namespace Memory
} // namespace ZeroCP

#endif // ZEROCP_MEMPOOL_MANAGER_HPP