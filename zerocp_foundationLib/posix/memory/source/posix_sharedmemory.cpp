#include "posix_sharedmemory.hpp"
#include "posix_call.hpp"
#include "logging.hpp"
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

// 将 AccessMode 和 OpenMode 转换为 POSIX 的 O_* 标志
int convertToOflags(AccessMode accessMode, OpenMode openMode) noexcept
{
    int flags = 0;
    
    // 根据访问模式设置标志
    switch (accessMode)
    {
        case AccessMode::ReadOnly:
            flags |= O_RDONLY;
            break;
        case AccessMode::WriteOnly:
            flags |= O_WRONLY;
            break;
        case AccessMode::ReadWrite:
            flags |= O_RDWR;
            break;
    }
    
    // 根据打开模式设置标志
    switch (openMode)
    {
        case OpenMode::ExclusiveCreate:
            flags |= O_CREAT | O_EXCL;
            break;
        case OpenMode::PurgeAndCreate:
            flags |= O_CREAT;
            break;
        case OpenMode::OpenOrCreate:
            flags |= O_CREAT;
            break;
        case OpenMode::OpenExisting:
            // 不需要额外标志
            break;
    }
    
    return flags;
}

std::string addLeadingSlash(const PosixSharedMemory::Name_t& name) noexcept
{
    // 创建带有前导斜杠的名称字符串
    std::string nameWithLeadingSlash = "/";
    // 将原始名称追加到前导斜杠后面
    nameWithLeadingSlash += name;
    return nameWithLeadingSlash;
}

std::expected<PosixSharedMemory, PosixSharedMemoryError> PosixSharedMemoryBuilder::create() noexcept
{
    if (m_name.empty())
    {
        ZEROCP_LOG(Error, "Shared memory name is empty");
        return std::unexpected(PosixSharedMemoryError::EMPTY_NAME);
    }
    
    auto nameWithLeadingSlash = addLeadingSlash(m_name);
    ZEROCP_LOG(Info, "Creating shared memory with name: " << nameWithLeadingSlash << ", size: " << m_memorySize);
    bool hasOwnership = ((m_openMode == OpenMode::ExclusiveCreate) || (m_openMode == OpenMode::PurgeAndCreate)
                         || m_openMode == OpenMode::OpenOrCreate);

    // 检查拥有所有权但访问模式为只读的不兼容情况
    if (hasOwnership && m_accessMode == AccessMode::ReadOnly)
    {
        ZEROCP_LOG(Error, "Cannot create shared-memory file \"" << m_name << "\" in read-only mode. "
                                                  << "Initializing a new file requires write access");
        return std::unexpected(PosixSharedMemoryError::INCOMPATIBLE_OPEN_AND_ACCESS_MODE);
    }
    
    // 如果是 PurgeAndCreate 模式，先删除已存在的共享内存
    if (m_openMode == OpenMode::PurgeAndCreate)
    {
        auto unlinkResult = ZeroCp_PosixCall(shm_unlink)(nameWithLeadingSlash.c_str())
            .failureReturnValue(-1)
            .evaluate();
        
        // 忽略删除失败的错误，因为可能共享内存不存在
        if (!unlinkResult.has_value() && unlinkResult.error().errnum != ENOENT)
        {
            ZEROCP_LOG(Warn, "Failed to unlink existing shared memory: " << strerror(unlinkResult.error().errnum));
        }
    }

    shm_handle_t sharedMemoryFileHandle = PosixSharedMemory::INVALID_HANDLE;
    
    // 尝试打开或创建共享内存对象
    auto result = ZeroCp_PosixCall(shm_open)(
        nameWithLeadingSlash.c_str(),
        convertToOflags(m_accessMode,
                        // 如果是OpenOrCreate模式，先尝试独占创建
                        (m_openMode == OpenMode::OpenOrCreate) ? OpenMode::ExclusiveCreate : m_openMode),
        to_mode(m_filePermissions))
        .failureReturnValue(PosixSharedMemory::INVALID_HANDLE)
        .evaluate();
    
    if (!result.has_value())
    {
        // 如果是OpenOrCreate模式且因为文件已存在而失败，则尝试以打开现有文件的方式重新打开
        if (m_openMode == OpenMode::OpenOrCreate && result.error().errnum == EEXIST)
        {
            hasOwnership = false;  // 文件已存在，我们不拥有所有权
            auto retryResult = ZeroCp_PosixCall(shm_open)(nameWithLeadingSlash.c_str(),
                                                          convertToOflags(m_accessMode, OpenMode::OpenExisting),
                                                          to_mode(m_filePermissions))
                                 .failureReturnValue(PosixSharedMemory::INVALID_HANDLE)
                                 .evaluate();
            
            if (retryResult.has_value())
            {
                sharedMemoryFileHandle = retryResult.value().value;
            }
            else
            {
                ZEROCP_LOG(Error, "Failed to open existing shared memory: " << strerror(retryResult.error().errnum));
                return std::unexpected(PosixSharedMemoryError::INVALID_NAME);
            }
        }
        else
        {
            // 根据 errno 返回相应的错误
            switch (result.error().errnum)
            {
                case EEXIST:
                    return std::unexpected(PosixSharedMemoryError::DOES_EXIST);
                case ENOENT:
                    return std::unexpected(PosixSharedMemoryError::INVALID_NAME);
                case EACCES:
                    return std::unexpected(PosixSharedMemoryError::INSUFFICIENT_PERMISSIONS);
                default:
                    ZEROCP_LOG(Error, "Failed to open shared memory: " << strerror(result.error().errnum));
                    return std::unexpected(PosixSharedMemoryError::UNKNOWN_ERROR);
            }
        }
    }
    else
    {
        sharedMemoryFileHandle = result.value().value;
        ZEROCP_LOG(Info, "shm_open succeeded, handle: " << sharedMemoryFileHandle);
    }

    // 如果拥有所有权，需要设置共享内存的大小
    if (hasOwnership)
    {
        // 使用ftruncate设置共享内存对象的大小
        auto ftruncateResult = ZeroCp_PosixCall(ftruncate)(sharedMemoryFileHandle, static_cast<off_t>(m_memorySize))
                                  .failureReturnValue(-1)
                                  .evaluate();
        
        if (!ftruncateResult.has_value())
        {
            ZEROCP_LOG(Error, "Failed to set shared memory size: " << strerror(ftruncateResult.error().errnum));

            // 如果设置大小失败，需要清理已创建的资源
            // 关闭文件描述符
            auto closeResult = ZeroCp_PosixCall(close)(sharedMemoryFileHandle)
                .failureReturnValue(-1)
                .evaluate();
            
            if (!closeResult.has_value())
            {
                ZEROCP_LOG(Error, "Unable to close filedescriptor (close failed): " 
                    << strerror(closeResult.error().errnum) << " for SharedMemory \"" << m_name << "\"");
            }

            // 删除已创建的共享内存对象
            auto unlinkResult = ZeroCp_PosixCall(shm_unlink)(nameWithLeadingSlash.c_str())
                .failureReturnValue(-1)
                .evaluate();
            
            if (!unlinkResult.has_value())
            {
                ZEROCP_LOG(Error, "Unable to remove previously created SharedMemory \""
                    << m_name << "\". This may be a SharedMemory leak.");
            }

            return std::unexpected(PosixSharedMemoryError::UNKNOWN_ERROR);
        }
    }
    
    // 创建共享内存对象
    return std::expected<PosixSharedMemory, PosixSharedMemoryError>(PosixSharedMemory(m_name, sharedMemoryFileHandle, hasOwnership));
}


uint64_t PosixSharedMemory::getMemorySize() const noexcept
{
    // 使用fstat获取实际的共享内存大小
    struct stat st;
    if (fstat(m_handle, &st) == 0)
    {
        return static_cast<uint64_t>(st.st_size);
    }
    else
    {
        ZEROCP_LOG(Error, "fstat failed for handle " << m_handle << ": " << strerror(errno));
    }
    return 0;
}

shm_handle_t PosixSharedMemory::getHandle() const
{
    return m_handle;
}

bool PosixSharedMemory::hasOwnership() const noexcept
{
    return m_hasOwnership;
}

PosixSharedMemory::~PosixSharedMemory()
{
    if (m_handle != INVALID_HANDLE)
    {
        auto closeResult = ZeroCp_PosixCall(close)(m_handle)
            .failureReturnValue(-1)
            .evaluate();
        
        if (!closeResult.has_value())
        {
            ZEROCP_LOG(Error, "Failed to close shared memory handle: " << strerror(closeResult.error().errnum));
        }
        
        // 如果拥有所有权，需要删除共享内存对象
        if (m_hasOwnership && !m_name.empty())
        {
            auto nameWithSlash = addLeadingSlash(m_name);
            auto unlinkResult = ZeroCp_PosixCall(shm_unlink)(nameWithSlash.c_str())
                .failureReturnValue(-1)
                .evaluate();
            
            if (!unlinkResult.has_value())
            {
                ZEROCP_LOG(Error, "Failed to unlink shared memory: " << strerror(unlinkResult.error().errnum));
            }
        }
    }
}

PosixSharedMemory::PosixSharedMemory(const Name_t& name, 
    const shm_handle_t handle, 
    const bool hasOwnership) noexcept
: m_name{name}          // 共享内存名称
, m_handle{handle}      // 文件句柄
, m_hasOwnership{hasOwnership}  // 是否拥有所有权
{
}

PosixSharedMemory::PosixSharedMemory(PosixSharedMemory&& other) noexcept
: m_name{std::move(other.m_name)}
, m_handle{other.m_handle}
, m_hasOwnership{other.m_hasOwnership}
{
    // 将源对象的句柄设置为无效，避免重复关闭
    other.m_handle = INVALID_HANDLE;
    other.m_hasOwnership = false;
}

PosixSharedMemory& PosixSharedMemory::operator=(PosixSharedMemory&& other) noexcept
{
    if (this != &other)
    {
        // 先清理当前对象的资源
        if (m_handle != INVALID_HANDLE)
        {
            close(m_handle);
            
            if (m_hasOwnership && !m_name.empty())
            {
                auto nameWithSlash = addLeadingSlash(m_name);
                shm_unlink(nameWithSlash.c_str());
            }
        }
        
        // 移动资源
        m_name = std::move(other.m_name);
        m_handle = other.m_handle;
        m_hasOwnership = other.m_hasOwnership;
        
        // 将源对象的句柄设置为无效
        other.m_handle = INVALID_HANDLE;
        other.m_hasOwnership = false;
    }
    return *this;
}

} // namespace Details
} // namespace ZeroCP
