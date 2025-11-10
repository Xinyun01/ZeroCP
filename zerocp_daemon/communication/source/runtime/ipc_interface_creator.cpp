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
IpcInterfaceCreator::createUnixDomainSocket(const RuntimeName_t& runtimeName, 
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
    // Build destination address: fixed server path (roudi)
    sockaddr_un dest{};
    dest.sun_family = AF_UNIX;
    const char* roudiPath = "udsServer.sock"; // fixed server socket name
    std::memset(dest.sun_path, 0, sizeof(dest.sun_path));
    std::strncpy(dest.sun_path, roudiPath, sizeof(dest.sun_path) - 1);
    auto sendRes = m_unixDomainSocket->sendTo(message, dest);
    if(!sendRes.has_value())
    {
        ZEROCP_LOG(Error, "Failed to send message to server. server=" << roudiPath
                     << " err=" << static_cast<int>(sendRes.error()));
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
    // 如果 RuntimeMessage 是字符串/字符串别名，直接赋值
    message = payload;
    // 接收成功后向来源地址发送 ACK
    const char* ack = "ACK";
    auto ackRes = m_unixDomainSocket->sendTo(ack, fromAddr);
    if (!ackRes.has_value())
    {
        ZEROCP_LOG(Warn, "Failed to send ACK. err=" << static_cast<int>(ackRes.error()));
        // 仍然认为接收成功，不因 ACK 失败而影响接收流程
    }
    return true;
}

} // namespace Runtime
} // namespace ZeroCP


