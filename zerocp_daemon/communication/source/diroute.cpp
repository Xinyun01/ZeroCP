#include "zerocp_foundationLib/report/include/logging.hpp"
#include "diroute.hpp"
#include "runtime/ipc_interface_creator.hpp"
#include "runtime/message_runtime.hpp"
#include <thread>
#include <sstream>
#include <unistd.h>

namespace ZeroCP
{
namespace Diroute
{

void Diroute::run() noexcept
{
    m_runMonitoringAndDiscoveryThread = true;
    
    // 初始化ProcessManager运行时，使用Diroute作为运行时名称
    ZeroCP::Runtime::RuntimeName_t runtimeName;
    runtimeName.insert(0, "DirouteRuntime");
    ZeroCP::Runtime::ProcessManager::initRuntime(runtimeName);
    
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
    // UDS会自动添加前导"/"，使用相对路径即可
    const char* socketPath = "udsServer.sock";
    ZEROCP_LOG(Info, "Creating server UDS at: " << socketPath);
    auto udsRes = creator.createUnixDomainSocket(serverName, ZeroCP::PosixIpcChannelSide::SERVER, socketPath);
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
        
        // 处理进程注册消息并传入creator用于发送响应
        handleProcessRegistration(message, creator);
    }
    // TODO: 放置消息接收与处理循环
}

/// 处理进程注册消息
void Diroute::handleProcessRegistration(const ZeroCP::Runtime::RuntimeMessage& message,
                                        ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept
{
    // 解析消息格式：假设消息格式为 "REGISTER:<processName>:<pid>:<isMonitored>"
    // 例如: "REGISTER:myApp:12345:1"
    std::istringstream iss(message);
    std::string command, processName, pidStr, monitoredStr;
    
    if (!std::getline(iss, command, ':') || command != "REGISTER")
    {
        ZEROCP_LOG(Warn, "Invalid message format, expected REGISTER command: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR: Invalid format";
        creator.sendMessage(response);
        return;
    }
    
    if (!std::getline(iss, processName, ':'))
    {
        ZEROCP_LOG(Error, "Failed to parse process name from message: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR: Invalid process name";
        creator.sendMessage(response);
        return;
    }
    
    if (!std::getline(iss, pidStr, ':'))
    {
        ZEROCP_LOG(Error, "Failed to parse PID from message: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR: Invalid PID";
        creator.sendMessage(response);
        return;
    }
    
    if (!std::getline(iss, monitoredStr))
    {
        ZEROCP_LOG(Error, "Failed to parse monitoring flag from message: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR: Invalid monitoring flag";
        creator.sendMessage(response);
        return;
    }
    
    // 转换数据
    uint32_t pid = 0;
    bool isMonitored = false;
    
    try
    {
        pid = std::stoul(pidStr);
        isMonitored = (monitoredStr == "1" || monitoredStr == "true");
    }
    catch (const std::exception& e)
    {
        ZEROCP_LOG(Error, "Failed to convert message data: " << e.what());
        ZeroCP::Runtime::RuntimeMessage response = "ERROR: Data conversion failed";
        creator.sendMessage(response);
        return;
    }
    
    // 构建RuntimeName_t
    ZeroCP::Runtime::RuntimeName_t runtimeName;
    runtimeName.insert(0, processName.c_str());
    
    // 调用ProcessManager单例注册进程（已有mutex保护，支持多客户端并发）
    auto& processMgr = ZeroCP::Runtime::ProcessManager::getInstance();
    bool success = processMgr.registerProcess(runtimeName, pid, isMonitored);
    
    if (success)
    {
        ZEROCP_LOG(Info, "Successfully registered process: " << processName 
                   << " (PID: " << pid << ", Monitored: " << isMonitored << ")");
        
        // 发送成功响应
        std::ostringstream responseStream;
        responseStream << "REGISTER_OK: Process " << processName << " registered successfully";
        ZeroCP::Runtime::RuntimeMessage response = responseStream.str();
        creator.sendMessage(response);
        ZEROCP_LOG(Info, "Sent confirmation to client: " << processName);
    }
    else
    {
        ZEROCP_LOG(Error, "Failed to register process: " << processName 
                   << " (PID: " << pid << ") - Process already registered");
        
        // 发送失败响应
        std::ostringstream responseStream;
        responseStream << "REGISTER_FAILED: Process " << processName << " already registered";
        ZeroCP::Runtime::RuntimeMessage response = responseStream.str();
        creator.sendMessage(response);
    }
}
}
}
