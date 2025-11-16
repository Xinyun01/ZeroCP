#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <unistd.h>
#include <sstream>
#include <new>

namespace ZeroCP
{
namespace Runtime
{

// 静态成员初始化
PoshRuntime* PoshRuntime::m_instance = nullptr;

// ============================================================================
// 静态方法
// ============================================================================

PoshRuntime& PoshRuntime::initRuntime(const RuntimeName_t& runtimeName) noexcept
{
    // 如果已经初始化，直接返回现有实例（不会重复注册）
    if (m_instance != nullptr)
    {
        ZEROCP_LOG(Warn, "PoshRuntime already initialized, returning existing instance (no re-registration)");
        return *m_instance;
    }
    
    ZEROCP_LOG(Info, "=== First-time PoshRuntime initialization ===");
    ZEROCP_LOG(Info, "Application name: " << runtimeName.c_str());
    
    // 静态缓冲区 + placement new（首次构造）
    alignas(PoshRuntime) static uint8_t runtimeBuffer[sizeof(PoshRuntime)];
    
    // 使用 placement new 在静态缓冲区创建对象
    // 构造函数中会自动：1) 创建IPC连接  2) 注册到守护进程
    m_instance = new (runtimeBuffer) PoshRuntime(runtimeName);
    
    ZEROCP_LOG(Info, "=== PoshRuntime initialization complete ===");
    return *m_instance;
}

PoshRuntime& PoshRuntime::getInstance() noexcept
{
    if (m_instance == nullptr)
    {
        ZEROCP_LOG(Error, "PoshRuntime not initialized! Call initRuntime() first");
        
        // 使用默认名称初始化
        RuntimeName_t defaultName;
        defaultName.insert(0, "DefaultApp");
        return initRuntime(defaultName);
    }
    
    return *m_instance;
}

// ============================================================================
// 构造和析构
// ============================================================================

PoshRuntime::PoshRuntime(const RuntimeName_t& runtimeName) noexcept
    : m_runtimeName(runtimeName)
    , m_pid(::getpid())
{
    ZEROCP_LOG(Info, ">>> PoshRuntime constructor (first-time construction) <<<");
    ZEROCP_LOG(Info, "Process: " << m_runtimeName.c_str() << ", PID: " << m_pid);
    
    // 步骤1: 初始化IPC连接
    ZEROCP_LOG(Info, "[Step 1/3] Initializing IPC connection...");
    if (!initializeConnection())
    {
        ZEROCP_LOG(Error, "Failed to initialize connection!");
        return;
    }
    
    // 步骤2: 自动注册到守护进程（首次构造时默认行为）
    ZEROCP_LOG(Info, "[Step 2/3] Automatically registering to RouteD daemon...");
    if (!registerToRouteD())
    {
        ZEROCP_LOG(Error, "Failed to register to RouteD!");
        return;
    }
    
    // 步骤3: 接收守护进程的响应
    ZEROCP_LOG(Info, "[Step 3/3] Receiving response from RouteD daemon...");
    if (!receiveRouteDAck())
    {
        ZEROCP_LOG(Error, "Failed to receive response from RouteD!");
        return;
    }
    
    m_isConnected = true;
    ZEROCP_LOG(Info, ">>> PoshRuntime ready and registered <<<");
}

PoshRuntime::~PoshRuntime() noexcept
{
    ZEROCP_LOG(Info, "PoshRuntime destructor - Process: " << m_runtimeName.c_str());
    m_isConnected = false;
}

// ============================================================================
// 公共接口
// ============================================================================

const RuntimeName_t& PoshRuntime::getRuntimeName() const noexcept
{
    return m_runtimeName;
}

bool PoshRuntime::sendMessage(const std::string& message) noexcept
{
    if (!m_isConnected || !m_ipcCreator)
    {
        ZEROCP_LOG(Error, "Cannot send message: not connected");
        return false;
    }
    
    RuntimeMessage msg = message;
    if (m_ipcCreator->sendMessage(msg))
    {
        ZEROCP_LOG(Debug, "Message sent: " << message);
        return true;
    }
    
    ZEROCP_LOG(Error, "Failed to send message: " << message);
    return false;
}

bool PoshRuntime::isConnected() const noexcept
{
    return m_isConnected;
}

// ============================================================================
// 私有方法
// ============================================================================

bool PoshRuntime::initializeConnection() noexcept
{
    ZEROCP_LOG(Info, "Creating IPC connection to RouteD...");
    
    try
    {
        m_ipcCreator = std::make_unique<IpcInterfaceCreator>();
        
        // 创建客户端UDS连接到守护进程
        // 客户端需要绑定到唯一的路径，而不是服务端路径
        std::string clientSocketPath = "client_" + std::to_string(m_pid) + ".sock";
        ZEROCP_LOG(Info, "Client binding to: " << clientSocketPath);
        auto result = m_ipcCreator->createUnixDomainSocket(
            m_runtimeName,
            PosixIpcChannelSide::CLIENT,
            clientSocketPath
        );
        
        if (!result.has_value())
        {
            ZEROCP_LOG(Error, "Failed to create Unix Domain Socket");
            return false;
        }
        
        ZEROCP_LOG(Info, "IPC connection created");
        return true;
    }
    catch (const std::exception& e)
    {
        ZEROCP_LOG(Error, "Exception in initializeConnection: " << e.what());
        return false;
    }
}

bool PoshRuntime::registerToRouteD() noexcept
{
    if (!m_ipcCreator)
    {
        ZEROCP_LOG(Error, "Cannot register: IPC not initialized");
        return false;
    }
    
    // 构造注册消息: "REGISTER:<processName>:<pid>:<isMonitored>"
    // isMonitored=1 表示需要守护进程监控此进程
    std::ostringstream oss;
    oss << "REGISTER:" << m_runtimeName.c_str() << ":" << m_pid << ":1";
    std::string registerMsg = oss.str();
    
    ZEROCP_LOG(Info, "Sending registration message to RouteD: " << registerMsg);
    
    RuntimeMessage msg = registerMsg;
    if (!m_ipcCreator->sendMessage(msg))
    {
        ZEROCP_LOG(Error, "Failed to send registration message");
        return false;
    }
    
    ZEROCP_LOG(Info, "✓ Registration message sent to RouteD daemon");
    return true;
}

bool PoshRuntime::receiveRouteDAck() noexcept
{
    if (!m_ipcCreator)
    {
        ZEROCP_LOG(Error, "Cannot receive: IPC not initialized");
        return false;
    }
    
    // 接收守护进程的响应
    RuntimeMessage response;
    if (!m_ipcCreator->receiveMessage(response))
    {
        ZEROCP_LOG(Warn, "Failed to receive response from RouteD");
        return false;
    }
    
    ZEROCP_LOG(Info, "Received response from RouteD: " << response.c_str());
    
    // 简单验证响应内容（可以根据协议扩展）
    std::string responseStr(response.c_str());
    if (responseStr.find("OK") != std::string::npos || 
        responseStr.find("SUCCESS") != std::string::npos)
    {
        ZEROCP_LOG(Info, "✓ Registration confirmed by RouteD daemon");
        return true;
    }
    
    ZEROCP_LOG(Info, "Response received (content: " << responseStr << ")");
    return true;
}

} // namespace Runtime
} // namespace ZeroCP
