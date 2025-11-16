#include "runtime/ipc_interface_creator.hpp"
#include <string>
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <sys/un.h>
#include <cstring>
namespace ZeroCP
{
namespace Runtime
{

//创建连接符
std::expected<void, ZeroCP::PosixIpcChannelError>
IpcInterfaceCreator::createUnixDomainSocket([[maybe_unused]] const RuntimeName_t& runtimeName, 
                                            const PosixIpcChannelSide& posixSide,
                                            const std::string& udsPath) noexcept
{
    // Build a filesystem path for AF_UNIX; ensure it's within sockaddr_un::sun_path limits
    using UnixDomainSocketBuilder = ZeroCP::Details::UnixDomainSocketBuilder;

    ZeroCP::Details::UnixDomainSocket::UdsName_t udsName;
    udsName.insert(0, udsPath.c_str());

    m_udsName_t = udsName;
    
    
    // store side
    m_unixDomainSocketSide = posixSide;

    auto resultUDS = UnixDomainSocketBuilder().name(m_udsName_t)
           .channelSide(posixSide)
           .maxMsgSize(ZeroCP::Details::UnixDomainSocket::MAX_MESSAGE_SIZE)
           .maxMsgNumber(ZeroCP::Details::UnixDomainSocket::MAX_MESSAGE_NUM)
           .create();
    if (!resultUDS.has_value())
    {
        ZEROCP_LOG(Error, "Failed to create UnixDomainSocket. udsPath=" << udsPath
                     << " side=" << (posixSide == PosixIpcChannelSide::SERVER ? "SERVER" : "CLIENT")
                     << " err=" << static_cast<int>(resultUDS.error()));
        return std::unexpected(resultUDS.error());
    }
    m_unixDomainSocket = std::move(resultUDS.value());
    ZEROCP_LOG(Info, "UnixDomainSocket created successfully. udsPath=" << udsPath
                    << " side=" << (posixSide == PosixIpcChannelSide::SERVER ? "SERVER" : "CLIENT"));
    return {};
}

bool IpcInterfaceCreator::sendMessage(const RuntimeMessage& message) noexcept
{
    sockaddr_un dest{};
    dest.sun_family = AF_UNIX;
    
    // 如果是服务器端且有客户端地址，发送到客户端
    if (m_unixDomainSocketSide == PosixIpcChannelSide::SERVER && m_hasClientAddr)
    {
        dest = m_lastClientAddr;
        ZEROCP_LOG(Debug, "Server sending response to client: " << dest.sun_path);
    }
    else  // 客户端发送到服务器
    {
        const char* serverPath = "udsServer.sock";
        std::memset(dest.sun_path, 0, sizeof(dest.sun_path));
        std::strncpy(dest.sun_path, serverPath, sizeof(dest.sun_path) - 1);
        ZEROCP_LOG(Debug, "Client sending message to server: " << serverPath);
    }
    
    auto sendRes = m_unixDomainSocket->sendTo(message, dest);
    if(!sendRes.has_value())
    {
        ZEROCP_LOG(Error, "Failed to send message. err=" << static_cast<int>(sendRes.error()));
        return false;
    }
    return true;
}


bool IpcInterfaceCreator::receiveMessage(RuntimeMessage& message) noexcept
{
    sockaddr_un fromAddr{};
    std::string payload;
    auto recvRes = m_unixDomainSocket->receiveFrom(payload, fromAddr);
    if (!recvRes.has_value())
    {
        ZEROCP_LOG(Error, "Failed to receive message. err=" << static_cast<int>(recvRes.error()));
        return false;
    }
    
    // 如果是服务器端，保存客户端地址用于发送响应
    if (m_unixDomainSocketSide == PosixIpcChannelSide::SERVER)
    {
        m_lastClientAddr = fromAddr;
        m_hasClientAddr = true;
        ZEROCP_LOG(Debug, "Server received message from client: " << fromAddr.sun_path);
    }
    
    // 如果 RuntimeMessage 是字符串/字符串别名，直接赋值
    message = payload;
    return true;
}

} // namespace Runtime
} // namespace ZeroCP


