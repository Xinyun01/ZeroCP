#include "posix_sharedmemory_object.hpp"
#include "logging.hpp"

// 用户代码
//    │
//    ├─→ PosixSharedMemoryObjectBuilder()
//    │      └─→ 内存分配：创建临时对象
//    │          m_name = ""
//    │          m_memorySizeInBytes = 0U
//    │          m_accessMode = ReadOnly
//    │          m_openMode = OpenExisting
//    │          m_baseAddressHint = nullopt
//    │          m_permissions = perms::none
//    │
//    ├─→ .name("/my_shm")
//    │      └─→ m_name = "/my_shm"
//    │
//    ├─→ .memorySizeInBytes(100)
//    │      └─→ m_memorySizeInBytes = 100
//    │
//    └─→ .create()
//           │
//           ├─→ [系统调用 1] shm_open("/my_shm", O_RDONLY, 0)
//           │      └─→ 返回 fd = 3
//           │
//           ├─→ [系统调用 2] fstat(fd=3, &statbuf)
//           │      └─→ statbuf.st_size = 4096
//           │
//           ├─→ [系统调用 3] mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd=3, 0)
//           │      └─→ 返回 addr = 0x7f1234567000
//           │
//           └─→ 返回 PosixSharedMemoryObject
//                  {
//                      文件描述符: 3
//                      映射地址: 0x7f1234567000
//                      映射大小: 4096
//                      所有权: false
//                  }
namespace ZeroCP
{
namespace Details
{


std::expected<PosixSharedMemoryObject, PosixSharedMemoryObjectError> PosixSharedMemoryObjectBuilder::create() noexcept
{
    auto SharedMemory = Details::PosixSharedMemoryBuilder()
                            .name(m_name)
                            .memorySize(m_memorySize)
                            .accessMode(m_accessMode)
                            .openMode(m_openMode)
                            .filePermissions(m_permissions)
                            .create();
    
    if (!SharedMemory)
    {
        ZEROCP_LOG(Error, "Failed to create shared memory object");
        return std::unexpected(PosixSharedMemoryObjectError::UNKNOWN_ERROR);
    }
    auto ResultSize = SharedMemory->getMemorySize();
    
    if (ResultSize == 0)
    {
        ZEROCP_LOG(Error,
                "Unable to create SharedMemoryObject since we could not acquire the memory size of the "
                "underlying object.");
        return std::unexpected(PosixSharedMemoryObjectError::UNABLE_TO_VERIFY_MEMORY_SIZE);
    }

    const auto realSize = ResultSize;
    if (realSize < m_memorySize)
    {
        ZEROCP_LOG(Error,
                "Unable to create SharedMemoryObject since a size of "
                    << m_memorySize << " was requested but the object has only a size of " << realSize);
        return std::unexpected(PosixSharedMemoryObjectError::REQUESTED_SIZE_EXCEEDS_ACTUAL_SIZE);
    }

    auto memoryMap = Details::PosixMemoryMapBuilder()
    .baseMemory((m_baseAddressHint) ? *m_baseAddressHint : nullptr)
    .memoryLength(realSize)
    .fileDescriptor(SharedMemory->getHandle())
    .prot((m_accessMode == AccessMode::ReadOnly) ? PROT_READ : (PROT_READ | PROT_WRITE))
    .flags(MAP_SHARED)
    .offset_(0)
    .create();

    if (!memoryMap)
    {
        ZEROCP_LOG(Error, "Failed to create memory map");
        return std::unexpected(PosixSharedMemoryObjectError::UNKNOWN_ERROR);
    }
    
    return std::expected<PosixSharedMemoryObject, PosixSharedMemoryObjectError>(
        PosixSharedMemoryObject(std::move(*SharedMemory), std::move(*memoryMap)));
}

PosixSharedMemoryObject::PosixSharedMemoryObject(Details::PosixSharedMemory&& sharedMemory,
    Details::PosixMemoryMap&& memoryMap) noexcept
: m_sharedMemory(std::move(sharedMemory))
, m_memoryMap(std::move(memoryMap))
{
}


// 获取基地址的常量版本
const void* PosixSharedMemoryObject::getBaseAddress() const noexcept
{
    return m_memoryMap.getBaseAddress();
}

// 获取基地址的非常量版本
void* PosixSharedMemoryObject::getBaseAddress() noexcept
{
    return m_memoryMap.getBaseAddress();
}
// 获取文件句柄
shm_handle_t PosixSharedMemoryObject::getFileHandle() const noexcept
{
    return m_sharedMemory.getHandle();
}

// 检查是否拥有共享内存的所有权
bool PosixSharedMemoryObject::hasOwnership() const noexcept
{
    return m_sharedMemory.hasOwnership();
}

} // namespace Details
} // namespace ZeroCP

