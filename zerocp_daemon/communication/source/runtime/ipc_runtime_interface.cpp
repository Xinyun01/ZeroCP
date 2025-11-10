#include "zerocp_daemon/communication/include/runtime/ipc_runtime_interface.hpp"

namespace ZeroCP
{
namespace Runtime
{

IpcRuntimeInterface::IpcRuntimeInterface(const RuntimeName_t& runtimeName) noexcept
    : m_runtimeName(runtimeName)
{
}

IpcRuntimeInterface::~IpcRuntimeInterface() noexcept
{
}

void IpcRuntimeInterface::registerRuntime(const RuntimeName_t& runtimeName) noexcept
{
}

void IpcRuntimeInterface::unregisterRuntime(const RuntimeName_t& runtimeName) noexcept
{
}

void IpcRuntimeInterface::sendRuntimeMessage(const RuntimeName_t& runtimeName, const RuntimeMessage& message) noexcept
{
}

void IpcRuntimeInterface::receiveRuntimeMessage(const RuntimeName_t& runtimeName, RuntimeMessage& message) noexcept
{
}

} // namespace Runtime
} // namespace ZeroCP
