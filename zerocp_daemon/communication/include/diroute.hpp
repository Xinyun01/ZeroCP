#ifndef ZEROCP_DIROUTE_HPP
#define ZEROCP_DIROUTE_HPP

#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include "zerocp_foundationLib/posix/memory/include/unix_domainsocket.hpp"
#include "runtime/ipc_interface_creator.hpp"
#include <thread>
#include <atomic>
namespace ZeroCP
{

namespace Diroute
{

using IpcInterfaceCreator_t = ZeroCP::Runtime::IpcInterfaceCreator;
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

private:
    std::thread m_startProcessRuntimeMessagesThread; // 监控与发现线程
    std::atomic<bool> m_runMonitoringAndDiscoveryThread{false};
};
} // namespace Diroute

} // namespace ZeroCP
#endif