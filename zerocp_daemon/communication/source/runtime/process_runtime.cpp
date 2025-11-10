#include "zerocp_daemon/communication/include/runtime/process_runtime.hpp"

namespace ZeroCP
{

RuntimeName_t ProcessRuntime::m_runtimeName{};

ProcessRuntime& ProcessRuntime::getInstance() noexcept
{
    static ProcessRuntime instance;
    return instance;
}

void ProcessRuntime::initRuntime(const RuntimeName_t& runtimeName) noexcept
{
    m_runtimeName = runtimeName;
    m_ipcRuntimeInterface = IpcRuntimeInterface(runtimeName);
}

void ProcessRuntime::registerRuntime(const RuntimeName_t& ) noexcept
{
    m_ipcRuntimeInterface.registerRuntime(runtimeName);
}

void ProcessRuntime::unregisterRuntime(const RuntimeName_t& ) noexcept
{
    m_ipcRuntimeInterface.unregisterRuntime(runtimeName);
}

} // namespace ZeroCP


