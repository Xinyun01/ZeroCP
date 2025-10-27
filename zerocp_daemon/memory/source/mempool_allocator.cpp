#include "mempool_allocator.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "filesystem.hpp"

namespace ZeroCP
{
namespace Memory
{

MemPoolAllocator::MemPoolAllocator(const MemPoolConfig& config) noexcept
    : m_config(config)
{
    // 获取或创建全局单例 MemPoolManager
    MemPoolManager& poolManager = MemPoolManager::getInstance(config);
    
    // 计算所需的总内存大小
    m_totalMemorySize = poolManager.getTotalMemorySize();
    
    ZEROCP_LOG(Info, "MemPoolAllocator 已初始化，计算得到总内存大小: " 
              << m_totalMemorySize << " 字节");
}

std::expected<void*, Details::PosixSharedMemoryObjectError> 
MemPoolAllocator::initializeSharedMemory(const std::string& shmName) noexcept
{
    // 检查是否已经初始化
    if (m_shmProvider != nullptr)
    {
        ZEROCP_LOG(Warn, "共享内存已经初始化，忽略重复初始化请求");
        return m_baseAddress;
    }
    
    // 检查内存大小是否有效
    if (m_totalMemorySize == 0)
    {
        ZEROCP_LOG(Error, "总内存大小为 0，无法创建共享内存");
        return std::unexpected(Details::PosixSharedMemoryObjectError::INVALID_SIZE);
    }

    ZEROCP_LOG(Info, "开始创建共享内存: " << shmName 
              << ", 大小: " << m_totalMemorySize << " 字节");

    // 创建 PosixShmProvider 实例
    // 使用 PurgeAndCreate 模式确保创建新的共享内存
    // 使用 ReadWrite 权限允许读写
    // 使用 OwnerAll 权限设置
    m_shmProvider = std::make_unique<PosixShmProvider>(
        shmName,
        m_totalMemorySize,
        AccessMode::ReadWrite,
        OpenMode::PurgeAndCreate,
        Perms::OwnerAll
    );

    // 创建共享内存并获取基地址
    auto result = m_shmProvider->createMemory();

    if (!result.has_value())
    {
        ZEROCP_LOG(Error, "创建共享内存失败: " << shmName);
        m_shmProvider.reset();  // 清理失败的实例
        return std::unexpected(result.error());
    }
    // 保存基地址
    m_baseAddress = result.value();
    
    ZEROCP_LOG(Info, "共享内存创建成功: " << shmName 
              << ", 基地址: " << m_baseAddress
              << ", 段ID: " << m_shmProvider->getSegmentId());
    
    // 通知内存可用
    m_shmProvider->announceMemoryAvailable();
    
    return m_baseAddress;
}

bool MemPoolAllocator::initializeMemPools() noexcept
{
 
    return true;
}



} // namespace Memory
} // namespace ZeroCP

