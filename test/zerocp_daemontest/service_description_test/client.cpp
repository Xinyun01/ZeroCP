#include "runtime/ipc_interface_creator.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <thread>
#include <chrono>

int main() {
    ZEROCP_LOG(Info, "ipc_client starting...");
    ZeroCP::Runtime::IpcInterfaceCreator creator;
    ZeroCP::Runtime::RuntimeName_t clientName;
    clientName.insert(0, "client");
    auto res = creator.createUnixDomainSocket(clientName, ZeroCP::PosixIpcChannelSide::CLIENT, "client.sock");
    if (!res.has_value()) {
        ZEROCP_LOG(Error, "createUnixDomainSocket failed for client. Ignore for test.");
    }

    ZeroCP::Runtime::RuntimeMessage msg = std::string("hello from client aaaaaa");
    creator.sendMessage(msg);
    // 接收服务器回复
    ZeroCP::Runtime::RuntimeMessage receivedMsg;
    if (creator.receiveMessage(receivedMsg)) {
        ZEROCP_LOG(Info, "Received reply: " << receivedMsg);
    } else {
        ZEROCP_LOG(Warn, "No reply received.");
    }

    ZEROCP_LOG(Info, "ipc_client done.");
    // Optionally try receive to keep symmetry; ignore result
    ZEROCP_LOG(Info, "ipc_client done.");
    // give server time to log
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return 0;
}
