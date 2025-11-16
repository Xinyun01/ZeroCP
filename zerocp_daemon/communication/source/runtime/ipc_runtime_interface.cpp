#include "zerocp_daemon/communication/include/runtime/ipc_runtime_interface.hpp"

namespace ZeroCP
{
namespace Runtime
{

IpcRuntimeInterface::IpcRuntimeInterface(const RuntimeName_t& runtimeName) noexcept
    : m_runtimeName(runtimeName)
    , m_messagesRuntime(runtimeName)
{
}

IpcRuntimeInterface::~IpcRuntimeInterface() noexcept
{
}

void IpcRuntimeInterface::registerRuntime([[maybe_unused]] const RuntimeName_t& runtimeName) noexcept
{
}

void IpcRuntimeInterface::unregisterRuntime([[maybe_unused]] const RuntimeName_t& runtimeName) noexcept
{
}

void IpcRuntimeInterface::sendRuntimeMessage([[maybe_unused]] const RuntimeName_t& runtimeName, [[maybe_unused]] const RuntimeMessage& message) noexcept
{
}

void IpcRuntimeInterface::receiveRuntimeMessage([[maybe_unused]] const RuntimeName_t& runtimeName, [[maybe_unused]] RuntimeMessage& message) noexcept
{
}

} // namespace Runtime
} // namespace ZeroCP
