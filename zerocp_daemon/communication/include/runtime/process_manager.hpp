#ifndef ZEROCP_PROCESS_RUNTIME_HPP
#define ZEROCP_PROCESS_RUNTIME_HPP

#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include "ipc_runtime_interface.hpp"
//当前代码是为了让每一个进程都注册一个名字app,对外部进程，然后通过路由端口发送到守护进程查看当前进程是否存在
#include "processinfo.hpp"
#include <vector>
#include <mutex>
namespace ZeroCP
{
using RuntimeName_t = ZeroCP::string<108>;
namespace Runtime
{
class ProcessManager
{
public:
    static void initRuntime(const RuntimeName_t& runtimeName) noexcept;
    static ProcessManager& getInstance() noexcept;

    ProcessManager(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) noexcept = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager& operator=(ProcessManager&&) noexcept = delete;
    ~ProcessManager() noexcept = default;

    bool registerProcess(const RuntimeName_t& name,
                        const uint32_t pid,
                        const bool isMonitored) noexcept;
    
    bool unregisterProcess(const RuntimeName_t& name) noexcept;
    
    bool isProcessRegistered(const RuntimeName_t& name) const noexcept;
    
    const ProcessInfo* getProcessInfo(const RuntimeName_t& name) const noexcept;
    
    const std::vector<ProcessInfo>& getAllProcesses() const noexcept;
    
    size_t getProcessCount() const noexcept;
    
    // 根据PID查询进程
    const ProcessInfo* getProcessInfoByPid(uint32_t pid) const noexcept;
    
    bool isProcessRegisteredByPid(uint32_t pid) const noexcept;
    
    // 监控管理
    std::vector<ProcessInfo> getMonitoredProcesses() const noexcept;
    
    bool updateProcessMonitoringStatus(const RuntimeName_t& name, bool isMonitored) noexcept;
    
    size_t getMonitoredProcessCount() const noexcept;
    
    // 批量操作
    void clearAllProcesses() noexcept;
    
    bool unregisterProcessByPid(uint32_t pid) noexcept;
    
    // 进程通信（通过IpcRuntimeInterface）
    bool sendMessageToProcess(const RuntimeName_t& name, const std::string& message) noexcept;
    
    // 进程信息更新
    bool updateProcessPid(const RuntimeName_t& name, uint32_t newPid) noexcept;
    
    // 显示进程信息
    void printAllProcesses() const noexcept;  // 打印所有进程到日志
    
    std::string getProcessListSummary() const noexcept;  // 获取进程列表摘要字符串
    
    void dumpProcessInfo() const noexcept;  // 详细导出所有进程信息
    
    std::string formatProcessInfo(const ProcessInfo& info) const noexcept;  // 格式化单个进程信息
    
private:
    ProcessManager() = default;
    explicit ProcessManager(const RuntimeName_t& runtimeName);
    
    static ProcessManager* m_instance;      // 单例指针（必须用指针）
    static RuntimeName_t m_runtimeName;
    IpcRuntimeInterface m_ipcRuntimeInterface; // 成员对象（不用指针）
    std::vector<ProcessInfo> m_processInfo;
    mutable std::mutex m_mutex;  // 保护 m_processInfo 的互斥锁（多客户端并发注册）
};
}
}   

#endif
