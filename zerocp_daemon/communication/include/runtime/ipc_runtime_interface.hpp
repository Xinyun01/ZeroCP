#ifndef ZEROCP_IPC_RUNTIME_INTERFACE_HPP
#define ZEROCP_IPC_RUNTIME_INTERFACE_HPP
//主要运行端口，去首次启动服务的时候，进行注册
#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include "runtime/message_runtime.hpp"
#include <string>
namespace ZeroCP
{
namespace Runtime
{
using RuntimeName_t = ZeroCP::string<108>;
using RuntimeMessage = std::string;
class IpcRuntimeInterface
{
public:
    IpcRuntimeInterface(const RuntimeName_t& runtimeName) noexcept;
    ~IpcRuntimeInterface() noexcept;

    void registerRuntime(const RuntimeName_t& runtimeName) noexcept;
    void unregisterRuntime(const RuntimeName_t& runtimeName) noexcept;
    void sendRuntimeMessage(const RuntimeName_t& runtimeName, const RuntimeMessage& message) noexcept;
    void receiveRuntimeMessage(const RuntimeName_t& runtimeName, RuntimeMessage& message) noexcept;
private:
    RuntimeName_t m_runtimeName;
    MessageRuntime m_messagesRuntime; 
};

} // namespace Runtime
} // namespace ZeroCP

#endif