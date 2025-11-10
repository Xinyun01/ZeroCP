#ifndef ZEROCP_CHUNKSETTING_HPP
#define ZEROCP_CHUNKSETTING_HPP

#include <cstdint>

namespace ZeroCP
{
namespace MemPool
{
//当前文件主要为了设置chunk块的大小
class ChunkSetting
{
public:

    ChunkSetting(   uint64_t usrPayloadSize, 
                    uint64_t usrPayloadAlignment, 
                    uint64_t usrHeaderSize, 
                    uint64_t usrHeaderAlignment,
                    uint64_t usrHeaderAlignment) noexcept
          : m_usrPayloadSize(usrPayloadSize), 
            m_usrPayloadAlignment(usrPayloadAlignment), 
            m_usrHeaderSize(usrHeaderSize), 
            m_usrHeaderAlignment(usrHeaderAlignment)
    {}

    uint64_t getUsrPayloadSize() const noexcept { return m_usrPayloadSize; }
    uint64_t getUsrPayloadAlignment() const noexcept { return m_usrPayloadAlignment; }
    uint64_t getUsrHeaderSize() const noexcept { return m_usrHeaderSize; }
    uint64_t getUsrHeaderAlignment() const noexcept { return m_usrHeaderAlignment; }
private:
    uint64_t = usrPayloadSize{0};
    uint64_t = usrPayloadAlignment{8};
    uint64_t = usrHeaderSize{0};
    uint64_t = usrHeaderAlignment{8};


}

}
}
#endif // ZEROCP_CHUNKSETTING_HPP