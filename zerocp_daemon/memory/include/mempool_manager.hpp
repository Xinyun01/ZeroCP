#ifndef ZEROCP_MEMPOOL_MANAGER_HPP
#define ZEROCP_MEMPOOL_MANAGER_HPP

#include "mempool_config.hpp"
#include "mempool.hpp"
#include "chunk_manager.hpp"
#include "vector.hpp"
#include "relative_pointer.hpp"
#include <pthread.h>
#include <semaphore.h>
#include <cstdint>
#include <memory>

// 前向声明
namespace ZeroCP {
namespace Memory {
class PosixShmProvider;
}
}

using ZeroCP::vector;

namespace ZeroCP
{
namespace Memory
{

/// @brief 内存池管理器（完全在共享内存中）
/// @details 职责：
///   1. 管理所有内存池实例
///   2. 计算所需共享内存大小
///   3. 提供内存分配/释放接口
///   4. 管理 chunk 引用计数
/// @note 整个对象存储在共享内存中，支持多进程访问
class MemPoolManager
{
public:
    // ==================== 单例模式（共享内存版本） ====================
    
    /// @brief 创建共享内存中的实例（服务端使用）
    /// @param config 内存池配置
    /// @return 成功返回 true
    static bool createSharedInstance(const MemPoolConfig& config) noexcept;
    
    /// @brief 连接到已存在的共享内存实例（客户端使用）
    /// @return 成功返回 true
    /// @note 客户端不需要提供配置，直接连接到服务端创建的共享内存
    static bool attachToSharedInstance() noexcept;
    
    /// @brief 获取已初始化的全局实例
    /// @return 全局实例指针，如果未初始化则返回 nullptr
    static MemPoolManager* getInstanceIfInitialized() noexcept;
    
    /// @brief 销毁共享实例（最后一个进程退出时调用）
    static void destroySharedInstance() noexcept;
    
    // 禁用拷贝和移动
    MemPoolManager(const MemPoolManager&) noexcept = delete;
    MemPoolManager(MemPoolManager&&) noexcept = delete;
    MemPoolManager& operator=(const MemPoolManager&) noexcept = delete;
    MemPoolManager& operator=(MemPoolManager&&) noexcept = delete;
    
    ~MemPoolManager() noexcept = default;

    /// @brief 初始化内存池管理器
    bool initialize() noexcept;

    // ==================== 内存大小计算 ====================
    
    /// @brief 计算管理区需要的内存大小（freeList + ChunkManager对象）
    uint64_t getManagementMemorySize() const noexcept;
    
    /// @brief 计算数据区需要的内存大小（所有chunks）
    uint64_t getChunkMemorySize() const noexcept;
    
    /// @brief 计算总共需要的内存大小（包括MemPoolManager对象本身）
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
    
    /// @brief 通过索引获取 ChunkManager（用于跨进程重建）
    /// @param index ChunkManager 在 ChunkManagerPool 中的索引
    /// @return ChunkManager 指针，失败返回 nullptr
    /// @note 用于接收端根据索引重建 ChunkManager 指针
    ChunkManager* getChunkManagerByIndex(uint32_t index) noexcept;
    
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
    // ==================== 私有构造函数 ====================
    
    /// @brief 构造函数（进程本地对象）
    /// @param config 配置对象引用
    explicit MemPoolManager(const MemPoolConfig& config) noexcept;

    // ==================== 成员变量 ====================
    
    const MemPoolConfig& m_config;                  ///< 配置引用
    vector<MemPool, 16> m_mempools;                 ///< 数据 chunk 池（最多16个）
    vector<MemPool, 1> m_chunkManagerPool;          ///< ChunkManager 对象池
       
    // ==================== 静态成员（进程本地） ====================
    
    /// @brief 单例实例指针（进程本地，指向本地堆对象）
    /// @warning 这是进程本地指针，不能跨进程使用！
    /// @details 每个进程都有自己的 s_instance，但它们管理的数据在共享内存中。
    ///          MemPoolManager 对象本身在进程堆上（new 创建），不在共享内存中。
    ///          关键：每个进程映射共享内存的虚拟地址可能不同，所以每个进程
    ///          都需要重新调用 ManagementMemoryLayout 和 ChunkMemoryLayout
    ///          来正确设置本进程地址空间中的指针。
    static MemPoolManager* s_instance;
    
    /// @brief 管理区共享内存基地址（进程本地，每个进程映射地址可能不同）
    /// @details 操作系统会将同一个共享内存段映射到不同进程的不同虚拟地址。
    ///          例如：进程A可能映射到0x7f00..., 进程B可能映射到0x7e00...
    ///          这就是为什么需要 RelativePointer 而不是绝对指针！
    static void* s_managementBaseAddress;
    
    /// @brief 数据区共享内存基地址（进程本地，每个进程映射地址可能不同）
    static void* s_chunkBaseAddress;
    
    static size_t s_managementMemorySize;           ///< 管理区共享内存大小
    static size_t s_chunkMemorySize;                ///< 数据区共享内存大小
    static sem_t* s_initSemaphore;                  ///< 初始化信号量（命名信号量）
    static const char* MGMT_SHM_NAME;               ///< 管理区共享内存名称
    static const char* CHUNK_SHM_NAME;              ///< 数据区共享内存名称
    static const char* SEM_NAME;                    ///< 信号量名称
    
    // 保存 PosixShmProvider 对象，防止共享内存被析构
    static std::unique_ptr<PosixShmProvider> s_mgmtProvider;
    static std::unique_ptr<PosixShmProvider> s_chunkProvider;
    
    // 标记当前进程是否是创建者（拥有所有权）
    static bool s_isOwner;
};

} // namespace Memory
} // namespace ZeroCP

#endif // ZEROCP_MEMPOOL_MANAGER_HPP