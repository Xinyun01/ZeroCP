#ifndef ZEROCP_IPC_INTERFACE_CREATOR_HPP
#define ZEROCP_IPC_INTERFACE_CREATOR_HPP

#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include "ipc_runtime_interface.hpp"
#include "zerocp_foundationLib/posix/memory/include/unix_domainsocket.hpp"
#include <expected>
#include <string>
#include <optional>

namespace ZeroCP
{

namespace Runtime
{

using UnixDomainSocket_t = ZeroCP::Details::UnixDomainSocket;
using PosixIpcChannelError_t = ZeroCP::PosixIpcChannelError;
class IpcInterfaceCreator
{
public:
    // Build UDS and store internally; on success, ownership is moved to m_unixDomainSocket
    std::expected<void, PosixIpcChannelError_t>
    createUnixDomainSocket(const RuntimeName_t& runtimeName,
                           const PosixIpcChannelSide& posixSide,
                           const std::string& udsPath) noexcept;

    bool sendMessage(const RuntimeMessage& message) noexcept;

    bool receiveMessage(RuntimeMessage& message) noexcept;

private:
    std::optional<UnixDomainSocket_t> m_unixDomainSocket;
    ZeroCP::Details::UnixDomainSocket::UdsName_t m_udsName_t{};
    ZeroCP::PosixIpcChannelSide m_unixDomainSocketSide {ZeroCP::PosixIpcChannelSide::CLIENT};

};
}
}

#endif