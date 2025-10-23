#ifndef ZEROC_MEMORY_CHUNK_HEADER_HPP
#define ZEROC_MEMORY_CHUNK_HEADER_HPP

namespace ZeroCP
{
namespace Memory
{

struct ChunkHeader
{


    uint32_t m_userHeaderSize{0U};
    uint8_t m_chunkHeaderVersion{CHUNK_HEADER_VERSION};
    // 保留用于未来功能并用于指示填充字节；目前未使用并设置为 '0'
    uint8_t m_reserved{0};
    // 目前只是一个占位符
    uint16_t m_userHeaderId{NO_USER_HEADER};
    uint64_t m_sequenceNumber{0U};
    // 整个块的大小，包括头
    uint64_t m_chunkSize{0U};
    uint64_t m_userPayloadSize{0U};
    uint32_t m_userPayloadAlignment{1U};
};


} // namespace Memory
} // namespace ZeroCP

#endif // ZEROC_MEMORY_CHUNK_HEADER_HPP