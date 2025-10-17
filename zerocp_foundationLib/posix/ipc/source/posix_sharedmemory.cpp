#include "posix_sharedmemory.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

namespace ZeroCP
{
namespace Details
{

string<PosixSharedMemory::Name_t::capacity() + 1> addLeadingSlash(const PosixSharedMemory::Name_t& name) noexcept
{
    // 创建带有前导斜杠的名称字符串
    string<PosixSharedMemory::Name_t::capacity() + 1> nameWithLeadingSlash = "/";
    // 将原始名称追加到前导斜杠后面，如果超出容量则截断
    nameWithLeadingSlash.append(TruncateToCapacity, name);
    return nameWithLeadingSlash;
}

expected<PosixSharedMemory, PosixSharedMemoryError> PosixSharedMemoryBuilder::create() noexcept
{
    if (m_name.empty())
    {
        ZEROCP_LOG(LogLevel::Error, "Shared memory name is empty");
        return err(PosixSharedMemoryError::EMPTY_NAME);
    }
    
    auto nameWithLeadingSlash = addLeadingSlash(m_name);
    bool hasOwnership = ((m_openMode == OpenMode::ExclusiveCreate) || (m_openMode == OpenMode::PurgeAndCreate)
                         || m_openMode == OpenMode::OpenOrCreate);

    // 检查拥有所有权但访问模式为只读的不兼容情况
    if (hasOwnership && m_accessMode == AccessMode::ReadOnly)
    {
        ZEROCP_LOG(LogLevel::Error, "Cannot create shared-memory file \"" << m_name << "\" in read-only mode. "
                                                  << "Initializing a new file requires write access");
        return err(PosixSharedMemoryError::INCOMPATIBLE_OPEN_AND_ACCESS_MODE);
    }
    
    // 如果是 PurgeAndCreate 模式，先删除已存在的共享内存
    if (m_openMode == OpenMode::PurgeAndCreate)
    {
        auto unlinkResult = ZeroCp_PosixCall(shm_unlink)(nameWithLeadingSlash.c_str())
            .failureReturnValue(-1);
        
        // 忽略删除失败的错误，因为可能共享内存不存在
        if (!unlinkResult.hasSuccess() && unlinkResult.getErrnum() != ENOENT)
        {
            ZEROCP_LOG(LogLevel::Warning, "Failed to unlink existing shared memory: " << strerror(unlinkResult.getErrnum()));
        }
    }

    shm_handle_t sharedMemoryFileHandle = PosixSharedMemory::INVALID_HANDLE;
    
    // 尝试打开或创建共享内存对象
    auto result = ZeroCp_PosixCall(shm_open)(
        nameWithLeadingSlash.c_str(),
        convertToOflags(m_accessMode,
                        // 如果是OpenOrCreate模式，先尝试独占创建
                        (m_openMode == OpenMode::OpenOrCreate) ? OpenMode::ExclusiveCreate : m_openMode),
        m_filePermissions.value())
        .failureReturnValue(PosixSharedMemory::INVALID_HANDLE);
    
    if (!result.hasSuccess())
    {
        // 如果是OpenOrCreate模式且因为文件已存在而失败，则尝试以打开现有文件的方式重新打开
        if (m_openMode == OpenMode::OpenOrCreate && result.getErrnum() == EEXIST)
        {
            hasOwnership = false;  // 文件已存在，我们不拥有所有权
            auto retryResult = ZeroCp_PosixCall(shm_open)(nameWithLeadingSlash.c_str(),
                                                          convertToOflags(m_accessMode, OpenMode::OpenExisting),
                                                          m_filePermissions.value())
                                 .failureReturnValue(PosixSharedMemory::INVALID_HANDLE);
            
            if (retryResult.hasSuccess())
            {
                sharedMemoryFileHandle = retryResult.getValue();
            }
            else
            {
                ZEROCP_LOG(LogLevel::Error, "Failed to open existing shared memory: " << strerror(retryResult.getErrnum()));
                return err(PosixSharedMemoryError::INVALID_NAME);
            }
        }
        else
        {
            // 根据 errno 返回相应的错误
            switch (result.getErrnum())
            {
                case EEXIST:
                    return err(PosixSharedMemoryError::DOES_EXIST);
                case ENOENT:
                    return err(PosixSharedMemoryError::INVALID_NAME);
                case EACCES:
                    return err(PosixSharedMemoryError::INSUFFICIENT_PERMISSIONS);
                default:
                    ZEROCP_LOG(LogLevel::Error, "Failed to open shared memory: " << strerror(result.getErrnum()));
                    return err(PosixSharedMemoryError::UNKNOWN_ERROR);
            }
        }
    }
    else
    {
        sharedMemoryFileHandle = result.getValue();
    }

    // 如果拥有所有权，需要设置共享内存的大小
    if (hasOwnership)
    {
        // 使用ftruncate设置共享内存对象的大小
        auto ftruncateResult = ZeroCp_PosixCall(ftruncate)(sharedMemoryFileHandle, static_cast<off_t>(m_size))
                                  .failureReturnValue(-1);
        
        if (!ftruncateResult.hasSuccess())
        {
            ZEROCP_LOG(LogLevel::Error, "Failed to set shared memory size: " << strerror(ftruncateResult.getErrnum()));

            // 如果设置大小失败，需要清理已创建的资源
            // 关闭文件描述符
            auto closeResult = ZeroCp_PosixCall(close)(sharedMemoryFileHandle)
                .failureReturnValue(-1);
            
            if (!closeResult.hasSuccess())
            {
                ZEROCP_LOG(LogLevel::Error, "Unable to close filedescriptor (close failed): " 
                    << strerror(closeResult.getErrnum()) << " for SharedMemory \"" << m_name << "\"");
            }

            // 删除已创建的共享内存对象
            auto unlinkResult = ZeroCp_PosixCall(shm_unlink)(nameWithLeadingSlash.c_str())
                .failureReturnValue(-1);
            
            if (!unlinkResult.hasSuccess())
            {
                ZEROCP_LOG(LogLevel::Error, "Unable to remove previously created SharedMemory \""
                    << m_name << "\". This may be a SharedMemory leak.");
            }

            return err(PosixSharedMemoryError::UNKNOWN_ERROR);
        }
    }
    
    // 创建共享内存对象
    return ok(PosixSharedMemory(m_name, sharedMemoryFileHandle, hasOwnership));
}

PosixSharedMemory::PosixSharedMemory(const Name_t& name, 
    const shm_handle_t handle, 
    const bool hasOwnership) noexcept
: m_name{name}          // 共享内存名称
, m_handle{handle}      // 文件句柄
, m_hasOwnership{hasOwnership}  // 是否拥有所有权
{
}

} // namespace Details
} // namespace ZeroCP
