#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "zerocp_daemon/diroute/diroute_components.hpp"
#include <unistd.h>
#include <sstream>
#include <new>
#include <chrono>

namespace ZeroCP
{
namespace Runtime
{

PoshRuntime* PoshRuntime::m_instance = nullptr;

PoshRuntime& PoshRuntime::initRuntime(const RuntimeName_t& runtimeName) noexcept
{
    if (m_instance != nullptr)
    {
        ZEROCP_LOG(Warn, "PoshRuntime already initialized");
        return *m_instance;
    }
    
    ZEROCP_LOG(Info, "Initializing PoshRuntime: " << runtimeName.c_str());
    
    alignas(PoshRuntime) static uint8_t runtimeBuffer[sizeof(PoshRuntime)];
    m_instance = new (runtimeBuffer) PoshRuntime(runtimeName);
    
    return *m_instance;
}

PoshRuntime& PoshRuntime::getInstance() noexcept
{
    if (m_instance == nullptr)
    {
        ZEROCP_LOG(Error, "PoshRuntime not initialized");
        RuntimeName_t defaultName;
        defaultName.insert(0, "DefaultApp");
        return initRuntime(defaultName);
    }
    
    return *m_instance;
}

PoshRuntime::PoshRuntime(const RuntimeName_t& runtimeName) noexcept
    : m_runtimeName(runtimeName)
    , m_pid(::getpid())
{
    if (!initializeConnection())
    {
        ZEROCP_LOG(Error, "Failed to initialize connection");
        return;
    }
    
    if (!registerToRouteD())
    {
        ZEROCP_LOG(Error, "Failed to register to RouteD");
        return;
    }
    
    if (!receiveRouteDAck())
    {
        ZEROCP_LOG(Error, "Failed to receive RouteD response");
        return;
    }
    
    m_isConnected = true;
    ZEROCP_LOG(Info, "PoshRuntime ready: " << m_runtimeName.c_str() << " (PID: " << m_pid << ")");
}

PoshRuntime::~PoshRuntime() noexcept
{
    stopHeartbeat();
    m_isConnected = false;
}

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
    if (!m_ipcCreator->sendMessage(msg))
    {
        ZEROCP_LOG(Error, "Failed to send message");
        return false;
    }
    
    return true;
}

bool PoshRuntime::isConnected() const noexcept
{
    return m_isConnected;
}

bool PoshRuntime::initializeConnection() noexcept
{
    try
    {
        m_ipcCreator = std::make_unique<IpcInterfaceCreator>();
        std::string clientSocketPath = "client_" + std::to_string(m_pid) + ".sock";
        
        auto result = m_ipcCreator->createUnixDomainSocket(
            m_runtimeName,
            PosixIpcChannelSide::CLIENT,
            clientSocketPath
        );
        
        if (!result.has_value())
        {
            ZEROCP_LOG(Error, "Failed to create UDS");
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
        ZEROCP_LOG(Error, "Connection exception: " << e.what());
        return false;
    }
}

bool PoshRuntime::registerToRouteD() noexcept
{
    if (!m_ipcCreator)
    {
        ZEROCP_LOG(Error, "IPC not initialized");
        return false;
    }
    
    std::ostringstream oss;
    oss << "REGISTER:" << m_runtimeName.c_str() << ":" << m_pid << ":1";
    
    RuntimeMessage msg = oss.str();
    if (!m_ipcCreator->sendMessage(msg))
    {
        ZEROCP_LOG(Error, "Failed to send registration");
        return false;
    }
    
    return true;
}

bool PoshRuntime::openHeartbeatSharedMemory() noexcept
{
    try
    {
        auto shmResult = ZeroCP::Details::PosixSharedMemoryObjectBuilder()
            .name("zerocp_diroute_components")
            .memorySize(sizeof(ZeroCP::Diroute::DirouteComponents))
            .accessMode(ZeroCP::AccessMode::ReadWrite)
            .openMode(ZeroCP::OpenMode::OpenExisting)
            .create();
        
        if (!shmResult)
        {
            ZEROCP_LOG(Error, "Failed to open shared memory");
            return false;
        }
        
        m_heartbeatShm = std::make_unique<ZeroCP::Details::PosixSharedMemoryObject>(std::move(*shmResult));
        return true;
    }
    catch (const std::exception& e)
    {
        ZEROCP_LOG(Error, "SHM exception: " << e.what());
        return false;
    }
}

bool PoshRuntime::registerHeartbeatSlot(uint64_t slotIndex) noexcept
{
    if (!m_heartbeatShm)
    {
        ZEROCP_LOG(Error, "Shared memory not opened");
        return false;
    }
    
    void* baseAddress = m_heartbeatShm->getBaseAddress();
    auto* components = reinterpret_cast<ZeroCP::Diroute::DirouteComponents*>(baseAddress);
    auto& heartbeatPool = components->heartbeatPool();
    auto it = heartbeatPool.iteratorFromIndex(slotIndex);
    
    if (it == heartbeatPool.end())
    {
        ZEROCP_LOG(Error, "Invalid heartbeat slot index: " << slotIndex);
        return false;
    }
    
    m_heartbeatSlot = &(*it);
    updateHeartbeat();
    
    return true;
}

void PoshRuntime::updateHeartbeat() noexcept
{
    if (m_heartbeatSlot)
    {
        m_heartbeatSlot->touch();
    }
}

void PoshRuntime::startHeartbeat() noexcept
{
    if (m_heartbeatRunning.load())
    {
        return;
    }
    
    m_heartbeatRunning.store(true);
    m_heartbeatThread = std::make_unique<std::thread>(&PoshRuntime::heartbeatThreadFunc, this);
}

void PoshRuntime::stopHeartbeat() noexcept
{
    if (m_heartbeatRunning.load())
    {
        m_heartbeatRunning.store(false);
        
        if (m_heartbeatThread && m_heartbeatThread->joinable())
        {
            m_heartbeatThread->join();
        }
    }
}

void PoshRuntime::heartbeatThreadFunc() noexcept
{
    while (m_heartbeatRunning.load())
    {
        updateHeartbeat();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool PoshRuntime::receiveRouteDAck() noexcept
{
    if (!m_ipcCreator)
    {
        ZEROCP_LOG(Error, "IPC not initialized");
        return false;
    }
    
    RuntimeMessage response;
    if (!m_ipcCreator->receiveMessage(response))
    {
        ZEROCP_LOG(Error, "Failed to receive response");
        return false;
    }
    
    std::string responseStr(response.c_str());
    
    if (responseStr.find("OK:OFFSET:") == 0)
    {
        size_t offsetPos = responseStr.find_last_of(':');
        if (offsetPos != std::string::npos)
        {
            std::string offsetStr = responseStr.substr(offsetPos + 1);
            m_heartbeatSlotIndex = std::stoull(offsetStr);
            
            ZEROCP_LOG(Info, "Heartbeat slot index: " << m_heartbeatSlotIndex);
            
            if (!openHeartbeatSharedMemory())
            {
                ZEROCP_LOG(Error, "Failed to open shared memory");
                return false;
            }
            
            if (!registerHeartbeatSlot(m_heartbeatSlotIndex))
            {
                ZEROCP_LOG(Error, "Failed to register slot");
                return false;
            }
            
            startHeartbeat();
            return true;
        }
    }
    
    ZEROCP_LOG(Error, "Unexpected response: " << responseStr);
    return false;
}

} // namespace Runtime
} // namespace ZeroCP
