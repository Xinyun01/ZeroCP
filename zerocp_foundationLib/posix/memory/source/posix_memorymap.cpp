#include "posix_memorymap.hpp"
#include "posix_call.hpp"
#include "logging.hpp"
#include <sys/mman.h>
#include <cstring>

namespace ZeroCP
{
namespace Details
{

// 将 AccessMode 转换为 PROT_* 标志
int convertToProtFlags(AccessMode accessMode) noexcept
{
    switch (accessMode)
    {
        case AccessMode::ReadOnly:
            return PROT_READ;
        case AccessMode::WriteOnly:
            return PROT_WRITE;
        case AccessMode::ReadWrite:
            return PROT_READ | PROT_WRITE;
        default:
            return PROT_NONE;
    }
}

// 重载版本：直接返回 int 类型的保护标志
int convertToProtFlags(int prot) noexcept
{
    return prot;
}

std::expected<PosixMemoryMap, PosixMemoryMapError> PosixMemoryMapBuilder::create() noexcept
{
    // void *mmap(void *addr,      // 建议的映射起始地址（进程虚拟地址空间中的地址），通常为NULL让内核自动选择
    //            size_t length,   // 映射的字节长度（必须>0）
    //            int prot,        // 内存保护标志：PROT_READ(可读), PROT_WRITE(可写), PROT_EXEC(可执行), PROT_NONE(不可访问)
    //            int flags,       // 映射标志：MAP_SHARED(共享), MAP_PRIVATE(私有), MAP_ANONYMOUS(匿名), MAP_FIXED(固定地址)
    //            int fd,          // 文件描述符（匿名映射时为-1）
    //            off_t offset);   // 文件偏移量（必须是页大小的整数倍）
    // 返回值：成功返回映射区域在进程虚拟地址空间中的起始地址，失败返回MAP_FAILED((void*)-1)并设置errno
    // 常见errno：EACCES(权限不足), EINVAL(参数无效), ENOMEM(内存不足), EBADF(无效文件描述符)
    // 注意：mmap处理的是进程虚拟地址，内核通过页表将其映射到物理内存，进程间可通过共享映射(MAP_SHARED)共享同一物理页
    auto result = ZeroCp_PosixCall(mmap)(const_cast<void*>(m_baseMemory),
                                       static_cast<size_t>(m_memoryLength),
                                       convertToProtFlags(m_prot),
                                       static_cast<int32_t>(m_flags),
                                       m_fileDescriptor,
                                       static_cast<off_t>(m_offset_))
                                    .failureReturnValue(MAP_FAILED)
                                    .evaluate();
    
    if(!result.has_value())   
    {
        ZEROCP_LOG(Error, "mmap failed: " << strerror(result.error().errnum));
        return std::unexpected(PosixMemoryMapError::ACCESS_FAILED);
    }
    
    return std::expected<PosixMemoryMap, PosixMemoryMapError>(PosixMemoryMap(result.value().value, m_memoryLength));
}


PosixMemoryMap::PosixMemoryMap(void* baseMemory, size_t memoryLength) noexcept
    : m_baseAddress(baseMemory), m_length(memoryLength)
{
}

// 析构函数已经在头文件中 default 了，这里不需要重复定义
// PosixMemoryMap::~PosixMemoryMap()
// {
//     if (m_baseAddress != nullptr)
//     {
//         // int munmap(void *addr,     // 要解除映射的起始地址（必须是之前mmap返回的地址）
//         //            size_t length); // 要解除映射的字节长度
//         // 功能：解除addr开始、长度为length的内存映射
//         // 返回值：成功返回0，失败返回-1并设置errno
//         // 常见errno：EINVAL(地址无效或不是页对齐)
//         auto unmapResult = ZeroCp_PosixCall(munmap)(m_baseAddress, static_cast<size_t>(m_length))
//                               .failureReturnValue(-1)
//                               .evaluate();
//
//         if (!unmapResult.has_value())
//         {
//             ZEROCP_LOG(Error,
//                     "Unable to unmap mapped memory [ address = " << m_baseAddress
//                                                                  << ", size = " << m_length << " ]");
//         }
//         
//         m_baseAddress = nullptr;
//         m_length = 0U;
//     }
// }

void* PosixMemoryMap::getBaseAddress() const noexcept
{
    return m_baseAddress;
}

uint64_t PosixMemoryMap::getLength() const noexcept
{
    return m_length;
}

PosixMemoryMap::PosixMemoryMap(PosixMemoryMap&& other) noexcept
: m_baseAddress{other.m_baseAddress}
, m_length{other.m_length}
{
    // 将源对象的地址设置为nullptr，避免重复释放
    other.m_baseAddress = nullptr;
    other.m_length = 0U;
}

PosixMemoryMap& PosixMemoryMap::operator=(PosixMemoryMap&& other) noexcept
{
    if (this != &other)
    {
        // 先清理当前对象的资源
        if (m_baseAddress != nullptr)
        {
            munmap(m_baseAddress, static_cast<size_t>(m_length));
        }
        
        // 移动资源
        m_baseAddress = other.m_baseAddress;
        m_length = other.m_length;
        
        // 将源对象的地址设置为nullptr
        other.m_baseAddress = nullptr;
        other.m_length = 0U;
    }
    return *this;
}

} // namespace Details
} // namespace ZeroCP

