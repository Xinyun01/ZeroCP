#ifndef ZEROCP_DIROUTE_HPP
#define ZEROCP_DIROUTE_HPP

#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include "zerocp_foundationLib/posix/memory/include/unix_domainsocket.hpp"
#include "runtime/ipc_interface_creator.hpp"
#include "runtime/process_manager.hpp"
#include "runtime/message_runtime.hpp"
#include <thread>
#include <atomic>
namespace ZeroCP
{
// RuntimeName_t 已在 process_manager.hpp 中定义为 string<108>
namespace Diroute
{

using IpcInterfaceCreator_t = ZeroCP::Runtime::IpcInterfaceCreator;
using ProcessManager = ZeroCP::Runtime::ProcessManager;
using RuntimeName_t = ZeroCP::Runtime::RuntimeName_t;  // 使用 Runtime 中的定义
class Diroute
{
public:
    Diroute() = default;
    Diroute(const Diroute& other) = delete;
    Diroute(Diroute&& other) noexcept = delete;
    Diroute& operator=(const Diroute& other) = delete;
    Diroute& operator=(Diroute&& other) noexcept = delete;
    ~Diroute() noexcept;
    void run() noexcept;
    void stop() noexcept;
    void startProcessRuntimeMessagesThread() noexcept;
    void processRuntimeMessagesThread() noexcept;
    void registerProcess(RuntimeName_t m_Runtime) noexcept;
private:
    void handleProcessRegistration(const ZeroCP::Runtime::RuntimeMessage& message,
                                    ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept;
    std::thread m_startProcessRuntimeMessagesThread; // 监控与发现线程
    std::atomic<bool> m_runMonitoringAndDiscoveryThread{false};
    MemPoolIntrospection m_memPoolIntrospection
};
} // namespace Diroute

} // namespace ZeroCP
#endif
