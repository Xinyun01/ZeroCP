#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/un.h>
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "zerocp_foundationLib/posix/memory/include/unix_domainsocket.hpp"

int main() {
    using namespace ZeroCP::Details;

    const pid_t pid = ::getpid();
    const std::string clientPath = "/tmp/zerocp_service_desc_client_" + std::to_string(pid) + ".sock";
    const std::string serverPath = "/tmp/zerocp_service_desc_server.sock";

    auto udsRes = UnixDomainSocketBuilder()
        .name(clientPath)
        .channelSide(PosixIpcChannelSide::CLIENT)
        .maxMsgSize(UnixDomainSocket::MAX_MESSAGE_SIZE)
        .maxMsgNumber(UnixDomainSocket::MAX_MESSAGE_NUM)
        .create();

    if (!udsRes.has_value()) {
        ZEROCP_LOG(Error, "Client: failed to create/bind UDS. err=" << static_cast<int>(udsRes.error()));
        return 1;
    }

    UnixDomainSocket client = std::move(udsRes.value());

    sockaddr_un dest{};
    dest.sun_family = AF_UNIX;
    if (serverPath.size() >= sizeof(dest.sun_path)) {
        ZEROCP_LOG(Error, "Client: server path too long");
        return 1;
    }
    std::memcpy(dest.sun_path, serverPath.c_str(), serverPath.size());
    dest.sun_path[serverPath.size()] = '\0';

    std::string msg = "hello_service_description";
    auto sendRes = client.sendTo(msg, dest);
    if (!sendRes.has_value()) {
        ZEROCP_LOG(Error, "Client: send failed err=" << static_cast<int>(sendRes.error()));
        return 1;
    }
    ZEROCP_LOG(Info, "Client sent: " << msg);

    std::string payload;
    sockaddr_un from{};
    auto recvRes = client.receiveFrom(payload, from);
    if (!recvRes.has_value()) {
        ZEROCP_LOG(Error, "Client: receive failed err=" << static_cast<int>(recvRes.error()));
        return 1;
    }
    ZEROCP_LOG(Info, "Client received: " << payload);
    return 0;
}


