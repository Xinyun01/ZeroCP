#ifndef ZEROCP_PROCESS_RUNTIME_HPP
#define ZEROCP_PROCESS_RUNTIME_HPP

#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include "ipc_runtime_interface.hpp"
//当前代码是为了让每一个进程都注册一个名字app,对外部进程，然后通过路由端口发送到守护进程查看当前进程是否存在
namespace ZeroCP
{
using RuntimeName_t = ZeroCP::string<108>;
namespace Runtime
{
class ProcessRuntime
{
public:
    static void initRuntime(const RuntimeName_t& runtimeName) noexcept;
    static ProcessRuntime& getInstance() noexcept;

    ProcessRuntime(const ProcessRuntime&) = delete;
    ProcessRuntime(ProcessRuntime&&) noexcept = delete;
    ProcessRuntime& operator=(const ProcessRuntime&) = delete;
    ProcessRuntime& operator=(ProcessRuntime&&) noexcept = delete;
    ~ProcessRuntime() noexcept = default;
    void registerRuntime(const RuntimeName_t& runtimeName) noexcept;
    void unregisterRuntime(const RuntimeName_t& runtimeName) noexcept;
private:
    ProcessRuntime() = default;
    static RuntimeName_t m_runtimeName;
    IpcRuntimeInterface m_ipcRuntimeInterface;
};
}
}   

#endif