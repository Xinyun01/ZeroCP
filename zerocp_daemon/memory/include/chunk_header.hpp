#ifndef ZEROC_MEMORY_CHUNK_HEADER_HPP
#define ZEROC_MEMORY_CHUNK_HEADER_HPP

#include <cstdint>

namespace ZeroCP
{
namespace Memory
{


/// @brief Chunk头结构
/// @note 每个chunk都以这个头开始，包含元数据信息
// ChunkHeader 是每个 Chunk 的元数据头
// 位置: iceoryx_posh/include/iceoryx_posh/mepoo/chunk_header.hpp

struct alignas(8) ChunkHeader
{
    // 用户头大小（可选的自定义头）
    uint32_t m_userHeaderSize{0U};
    

    // 保留字节（未来扩展）
    uint8_t m_reserved{0};
        
    // 发送者 ID（哪个端口发送的）
    // TODO: 需要定义 popo::UniquePortId
    // popo::UniquePortId m_originId{popo::InvalidPortId};
    uint64_t m_originId{0};
    
    // 序列号（发送顺序）
    uint64_t m_sequenceNumber{0U};
    
    // 整个 Chunk 的大小
    uint64_t m_chunkSize{0U};
    
    // 用户数据的大小
    uint64_t m_userPayloadSize{0U};
    
    // 用户数据的对齐要求
    uint32_t m_userPayloadAlignment{8U};
    
    // 用户数据相对于 ChunkHeader 的偏移量
    uint64_t m_userPayloadOffset{sizeof(ChunkHeader)};
};


} // namespace Memory
} // namespace ZeroCP

#endif // ZEROC_MEMORY_CHUNK_HEADER_HPP