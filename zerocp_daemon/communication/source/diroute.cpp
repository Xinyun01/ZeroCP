#include "zerocp_foundationLib/report/include/logging.hpp"
#include "diroute.hpp"
#include "runtime/ipc_interface_creator.hpp"
#include <thread>

namespace ZeroCP
{
namespace Diroute
{

void Diroute::run() noexcept
{
    m_runMonitoringAndDiscoveryThread = true;
    startProcessRuntimeMessagesThread();
}
    
Diroute::~Diroute() noexcept
{
    stop();
}

void Diroute::stop() noexcept
{
    m_runMonitoringAndDiscoveryThread = false;
    if (m_startProcessRuntimeMessagesThread.joinable())
    {
        m_startProcessRuntimeMessagesThread.join();
    }
}
// 启动进程运行时消息处理线程    
void Diroute::startProcessRuntimeMessagesThread() noexcept
{
    m_startProcessRuntimeMessagesThread = std::thread(&Diroute::processRuntimeMessagesThread, this);
}

/// 进程运行时消息处理线程主循环
void Diroute::processRuntimeMessagesThread() noexcept
{
    // 在工作线程中创建并绑定服务端 UDS
    ZeroCP::Runtime::IpcInterfaceCreator creator;
    ZeroCP::Runtime::RuntimeName_t serverName;
    serverName.insert(0, "udsServer");
    auto udsRes = creator.createUnixDomainSocket(serverName, ZeroCP::PosixIpcChannelSide::SERVER, "udsServer.sock");
    if (!udsRes.has_value())
    {
        ZEROCP_LOG(Error, "Failed to create server UDS in runtime thread.");
        return;
    }
    while(m_runMonitoringAndDiscoveryThread)
    {
        ZeroCP::Runtime::RuntimeMessage message;
        auto receiveRes = creator.receiveMessage(message);
        if (!receiveRes)
        {
            ZEROCP_LOG(Error, "Failed to receive message in runtime thread.");
            continue;
        }
        ZEROCP_LOG(Info, "Received message in runtime thread: " << message.c_str());
    }
    // TODO: 放置消息接收与处理循环
}
}
}