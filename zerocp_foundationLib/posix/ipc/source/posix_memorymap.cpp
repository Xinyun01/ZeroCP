#include "posix_memorymap.hpp"
#include "posix_call.hpp"
#include "logging.hpp"
#include <sys/mman.h>
#include <cstring>
// void *mmap(void *addr,           // 建议的映射地址
//     size_t length,        // 映射长度
//     int prot,             // 保护模式
//     int flags,            // 映射标志
//     int fd,               // 文件描述符
//     off_t offset);        // 文件偏移
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

PosixMemoryMap::~PosixMemoryMap()
{
    if (m_baseAddress != nullptr)
    {
        auto unmapResult = ZeroCp_PosixCall(munmap)(m_baseAddress, static_cast<size_t>(m_length))
                              .failureReturnValue(-1)
                              .evaluate();

        if (!unmapResult.has_value())
        {
            ZEROCP_LOG(Error,
                    "Unable to unmap mapped memory [ address = " << m_baseAddress
                                                                 << ", size = " << m_length << " ]");
        }
        
        m_baseAddress = nullptr;
        m_length = 0U;
    }
}

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

