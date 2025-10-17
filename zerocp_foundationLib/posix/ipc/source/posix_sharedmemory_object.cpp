#include "posix_sharedmemory_object.hpp"

namespace ZeroCP
{
namespace Details
{


expected<PosixSharedMemoryObject, PosixSharedMemoryObjectError> PosixSharedMemoryObjectBuilder::create() noexcept
{
    auto SharedMemory = PosixSharedMemory::name(m_name).memorySize(m_memorySize)
                                                        .accessMode(m_accessMode)
                                                        .openMode(m_openMode)
                                                        .permissions(m_permissions)
                                                        .create();
    
    if (!SharedMemory)
    {
        ZEROCP_LOG(LogLevel::Error, "Failed to create shared memory object: " << SharedMemory.error());
        return err(PosixSharedMemoryObjectError::UNKNOWN_ERROR);
    }
    auto ResultSize = SharedMemory->getMemorySize();
    
}

// 实现 PosixSharedMemoryObject 的接口方法











}
}

