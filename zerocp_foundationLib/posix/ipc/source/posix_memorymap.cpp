#include "posix_memorymap.hpp"
#include "posix_call.hpp"
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

expected<PosixMemoryMap, PosixMemoryMapError> PosixMemoryMapBuilder::create() noexcept
{
    auto result = ZeroCp_PosixCall(mmap)(const_cast<void*>(m_baseAddressHint),
                                       static_cast<size_t>(m_memoryLength),
                                       convertToProtFlags(m_prot),
                                       static_cast<int32_t>(m_flags),
                                       m_fileDescriptor,
                                       m_offset)
                                    .failureReturnValue(MAP_FAILED)
                                    .successReturnValue(PosixMemoryMap(result.getValue(), m_length));
    if(!result.hasValue())   
    {
        return err(PosixMemoryMapError::ACCESS_FAILED);
    }
    return PosixMemoryMap(result.getValue(), m_memoryLength);
}


PosixMemoryMap::PosixMemoryMap(void* baseMemory, size_t memoryLength) noexcept
    : m_baseMemory(m_baseAddress), m_memorySize(m_length)
{
}

PosixMemoryMap::~PosixMemoryMap()
{
    if (m_baseAddress != nullptr)
    {
        auto unmapResult = ZeroCp_PosixCall(munmap)(m_baseAddress, static_cast<size_t>(m_length)).failureReturnValue(-1).evaluate();
        m_baseAddress = nullptr;
        m_length = 0U;

        if (unmapResult.has_error())
        {
            errnoToEnum(unmapResult.error().errnum);
            ZeroCP_Log(Error,
                    "unable to unmap mapped memory [ address = " << iox::log::hex(m_baseAddress)
                                                                 << ", size = " << m_length << " ]");
            return false;
        }
    }

}
}

}

