#ifndef ZEROCP_DAEMON_POSIXSHM_PROVIDER_HPP
#define ZEROCP_DAEMON_POSIXSHM_PROVIDER_HPP

#include "posix_sharedmemory.hpp"
#include "posix_sharedmemory_object.hpp"
#include "logging.hpp"
#include <expected>
#include <optional>

namespace ZeroCP
{
namespace Memory
{

// 使用类型别名
using Name_t = Details::PosixSharedMemory::Name_t;
using ZeroCP::AccessMode;
using ZeroCP::OpenMode;
using ZeroCP::Perms;
using PosixSharedMemoryObject = Details::PosixSharedMemoryObject;
using PosixSharedMemoryObjectError = Details::PosixSharedMemoryObjectError;

// PosixShmProvider类：共享内存的创建、销毁和状态管理
class PosixShmProvider
{
public:
    // 构造函数：初始化共享内存名称、大小、访问模式、打开模式和权限
    PosixShmProvider(const Name_t& name, 
                    const uint64_t memorySize, 
                    const AccessMode accessMode, 
                    const OpenMode openMode, 
                    const Perms permissions) noexcept;
    
    // 析构函数：自动清理共享内存资源
    ~PosixShmProvider();

    // 禁用拷贝构造和移动构造（确保唯一性，防止意外复制共享内存提供者）
    PosixShmProvider(const PosixShmProvider&) = delete;
    PosixShmProvider(PosixShmProvider&&) noexcept = delete;
    PosixShmProvider& operator=(const PosixShmProvider&) = delete;
    PosixShmProvider& operator=(PosixShmProvider&&) noexcept = delete;
    
    // 创建共享内存，返回基地址
    std::expected<void*, PosixSharedMemoryObjectError> createMemory() noexcept;

    // 销毁共享内存
    std::expected<void, PosixSharedMemoryObjectError> destroyMemory() noexcept;
    
    // 获取内存池ID
    uint64_t getPoolId() const noexcept;
    
    // 判断共享内存是否可用
    bool isMemoryAvailable() const noexcept;
    
    // 获取共享内存的基地址
    void* getBaseAddress() const noexcept;
    
    /// 通知所有 MemoryBlock 已经可以使用分配好的内存
    void announceMemoryAvailable() noexcept;
private:
    Name_t m_name;                                      // 共享内存名称
    uint64_t m_memorySize{0U};                          // 共享内存大小（字节）
    AccessMode m_accessMode{AccessMode::ReadOnly};      // 访问模式
    OpenMode m_openMode{OpenMode::OpenExisting};        // 打开模式
    Perms m_permissions{Perms::OwnerAll};               // 权限设置
    std::optional<PosixSharedMemoryObject> m_sharedMemoryObject; // 共享内存对象
    void* m_baseAddress{nullptr};                       // 共享内存基地址
    bool m_memoryAvailableAnnounced{false};             // 内存可用标志，是否已通知各MemoryBlock
    uint64_t m_poolId{0};                               // 池ID
};

} // namespace Memory
} // namespace ZeroCP

#endif