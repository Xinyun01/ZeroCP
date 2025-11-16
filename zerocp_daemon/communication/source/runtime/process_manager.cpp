#include "zerocp_daemon/communication/include/runtime/process_manager.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <sstream>
#include <iomanip>

namespace ZeroCP
{
namespace Runtime
{

// 静态成员初始化
ProcessManager* ProcessManager::m_instance = nullptr;
RuntimeName_t ProcessManager::m_runtimeName;

// 单例模式实现
void ProcessManager::initRuntime(const RuntimeName_t& runtimeName) noexcept
{
    if (m_instance == nullptr)
    {
        m_runtimeName = runtimeName;
        m_instance = new ProcessManager(runtimeName);
        ZEROCP_LOG(Info, "ProcessManager initialized with runtime name: " << runtimeName.c_str());
    }
    else
    {
        ZEROCP_LOG(Warn, "ProcessManager already initialized.");
    }
}

ProcessManager& ProcessManager::getInstance() noexcept
{
    if (m_instance == nullptr)
    {
        ZEROCP_LOG(Error, "ProcessManager not initialized. Call initRuntime() first.");
        // 使用默认名称初始化
        RuntimeName_t defaultName;
        defaultName.insert(0, "DefaultRuntime");
        initRuntime(defaultName);
    }
    return *m_instance;
}

ProcessManager::ProcessManager(const RuntimeName_t& runtimeName)
    : m_ipcRuntimeInterface(runtimeName)  // 直接初始化成员对象
{
    ZEROCP_LOG(Info, "ProcessManager IpcRuntimeInterface initialized.");
}
bool ProcessManager::registerProcess(const RuntimeName_t& name,
                                        const uint32_t pid,
                                        const bool isMonitored ) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);  // 加锁保护，防止多客户端并发注册冲突
    
    for(const auto& it : m_processInfo)
    {
        if (std::strcmp(it.name.c_str(), name.c_str()) == 0)
        {
            ZEROCP_LOG(Error, "Failed to registerProcess because of name.");
            return false;      // 找到了
        }
    }
    m_processInfo.emplace_back(name, pid, isMonitored);
    ZEROCP_LOG(Info, "Process registered: " << name.c_str() << " (PID: " << pid << ")");
    return true;
}

bool ProcessManager::unregisterProcess(const RuntimeName_t& name) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);  // 加锁保护
    
    auto it = m_processInfo.begin();
    while (it != m_processInfo.end())
    {
        if (std::strcmp(it->name.c_str(), name.c_str()) == 0)
        {
            m_processInfo.erase(it);  // 从列表中删除这一项
            ZEROCP_LOG(Info, "Process unregistered: " << name.c_str());
            return true;              // 删除成功
        }
        ++it;
    }
    return false;
}

bool ProcessManager::isProcessRegistered(const RuntimeName_t& name) const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);  // 加锁保护读取
    for(const auto& processInfo : m_processInfo)
    {
        if (std::strcmp(processInfo.name.c_str(), name.c_str()) == 0)
        {
            return true;
        }
    }
    return false;
}

const ProcessInfo* ProcessManager::getProcessInfo(const RuntimeName_t& name) const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);  // 加锁保护读取
    for (const auto& processInfo : m_processInfo)
    {
        if (std::strcmp(processInfo.name.c_str(), name.c_str()) == 0)
        {
            return &processInfo;
        }
    }
    return nullptr;
}

const std::vector<ProcessInfo>& ProcessManager::getAllProcesses() const noexcept
{
    return m_processInfo;
}

size_t ProcessManager::getProcessCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);  // 加锁保护读取
    return m_processInfo.size();
}

// 根据PID查询进程
const ProcessInfo* ProcessManager::getProcessInfoByPid(uint32_t pid) const noexcept
{
    for (const auto& processInfo : m_processInfo)
    {
        if (processInfo.pid == pid)
        {
            return &processInfo;
        }
    }
    return nullptr;
}

bool ProcessManager::isProcessRegisteredByPid(uint32_t pid) const noexcept
{
    return getProcessInfoByPid(pid) != nullptr;
}

// 监控管理
std::vector<ProcessInfo> ProcessManager::getMonitoredProcesses() const noexcept
{
    std::vector<ProcessInfo> monitoredProcesses;
    for (const auto& processInfo : m_processInfo)
    {
        if (processInfo.isMonitored)
        {
            monitoredProcesses.push_back(processInfo);
        }
    }
    return monitoredProcesses;
}

bool ProcessManager::updateProcessMonitoringStatus(const RuntimeName_t& name, bool isMonitored) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& processInfo : m_processInfo)
    {
        if (std::strcmp(processInfo.name.c_str(), name.c_str()) == 0)
        {
            processInfo.setMonitored(isMonitored);
            ZEROCP_LOG(Info, "Updated monitoring status for process: " << name.c_str() 
                       << " to " << (isMonitored ? "true" : "false"));
            return true;
        }
    }
    ZEROCP_LOG(Warn, "Process not found: " << name.c_str());
    return false;
}

size_t ProcessManager::getMonitoredProcessCount() const noexcept
{
    size_t count = 0;
    for (const auto& processInfo : m_processInfo)
    {
        if (processInfo.isMonitored)
        {
            ++count;
        }
    }
    return count;
}

// 批量操作
void ProcessManager::clearAllProcesses() noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = m_processInfo.size();
    m_processInfo.clear();
    ZEROCP_LOG(Info, "Cleared all processes. Total removed: " << count);
}

bool ProcessManager::unregisterProcessByPid(uint32_t pid) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_processInfo.begin();
    while (it != m_processInfo.end())
    {
        if (it->pid == pid)
        {
            ZEROCP_LOG(Info, "Unregistered process by PID: " << pid);
            m_processInfo.erase(it);
            return true;
        }
        ++it;
    }
    ZEROCP_LOG(Warn, "Process with PID " << pid << " not found.");
    return false;
}

// 进程通信
bool ProcessManager::sendMessageToProcess(const RuntimeName_t& name, const std::string& message) noexcept
{
    // isProcessRegistered 内部已有锁保护
    if (!isProcessRegistered(name))
    {
        ZEROCP_LOG(Error, "Cannot send message: process not registered: " << name.c_str());
        return false;
    }
    
    // 直接使用成员对象
    m_ipcRuntimeInterface.sendRuntimeMessage(name, message);
    ZEROCP_LOG(Info, "Sent message to process: " << name.c_str());
    return true;
}

// 进程信息更新
bool ProcessManager::updateProcessPid(const RuntimeName_t& name, uint32_t newPid) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& processInfo : m_processInfo)
    {
        if (std::strcmp(processInfo.name.c_str(), name.c_str()) == 0)
        {
            uint32_t oldPid = processInfo.pid;
            processInfo.pid = newPid;
            ZEROCP_LOG(Info, "Updated PID for process: " << name.c_str() 
                       << " from " << oldPid << " to " << newPid);
            return true;
        }
    }
    ZEROCP_LOG(Warn, "Process not found: " << name.c_str());
    return false;
}

// 格式化单个进程信息
std::string ProcessManager::formatProcessInfo(const ProcessInfo& info) const noexcept
{
    std::ostringstream oss;
    oss << "Name: " << std::setw(20) << std::left << info.name.c_str()
        << " | PID: " << std::setw(8) << info.pid
        << " | Monitored: " << (info.isMonitored ? "Yes" : "No ");
    return oss.str();
}

// 打印所有进程到日志
void ProcessManager::printAllProcesses() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);  // 加锁保护读取
    ZEROCP_LOG(Info, "========== Connected Processes ==========");
    ZEROCP_LOG(Info, "Total processes: " << m_processInfo.size());
    ZEROCP_LOG(Info, "Monitored processes: " << getMonitoredProcessCount());
    ZEROCP_LOG(Info, "-----------------------------------------");
    
    if (m_processInfo.empty())
    {
        ZEROCP_LOG(Info, "No processes connected.");
    }
    else
    {
        for (const auto& processInfo : m_processInfo)
        {
            ZEROCP_LOG(Info, formatProcessInfo(processInfo));
        }
    }
    ZEROCP_LOG(Info, "=========================================");
}

// 获取进程列表摘要字符串
std::string ProcessManager::getProcessListSummary() const noexcept
{
    std::ostringstream oss;
    oss << "========== Process List Summary ==========\n";
    oss << "Total Processes: " << m_processInfo.size() << "\n";
    oss << "Monitored: " << getMonitoredProcessCount() << "\n";
    oss << "------------------------------------------\n";
    
    if (m_processInfo.empty())
    {
        oss << "No processes connected.\n";
    }
    else
    {
        oss << std::left << std::setw(25) << "Process Name" 
            << std::setw(10) << "PID" 
            << std::setw(12) << "Monitored" << "\n";
        oss << std::string(47, '-') << "\n";
        
        for (const auto& processInfo : m_processInfo)
        {
            oss << std::left << std::setw(25) << processInfo.name.c_str()
                << std::setw(10) << processInfo.pid
                << std::setw(12) << (processInfo.isMonitored ? "Yes" : "No")
                << "\n";
        }
    }
    oss << "==========================================";
    return oss.str();
}

// 详细导出所有进程信息
void ProcessManager::dumpProcessInfo() const noexcept
{
    ZEROCP_LOG(Info, "========== Detailed Process Information ==========");
    ZEROCP_LOG(Info, "Runtime Name: " << m_runtimeName.c_str());
    ZEROCP_LOG(Info, "Total Processes: " << m_processInfo.size());
    ZEROCP_LOG(Info, "==================================================");
    
    if (m_processInfo.empty())
    {
        ZEROCP_LOG(Info, "No processes registered.");
        return;
    }
    
    size_t index = 1;
    for (const auto& processInfo : m_processInfo)
    {
        ZEROCP_LOG(Info, "");
        ZEROCP_LOG(Info, "[Process #" << index << "]");
        ZEROCP_LOG(Info, "  Name       : " << processInfo.name.c_str());
        ZEROCP_LOG(Info, "  PID        : " << processInfo.pid);
        ZEROCP_LOG(Info, "  Monitored  : " << (processInfo.isMonitored ? "Yes" : "No"));
        ZEROCP_LOG(Info, "  Status     : " << (isProcessRegistered(processInfo.name) ? "Active" : "Inactive"));
        ++index;
    }
    
    ZEROCP_LOG(Info, "");
    ZEROCP_LOG(Info, "==================================================");
    ZEROCP_LOG(Info, "Summary Statistics:");
    ZEROCP_LOG(Info, "  - Monitored processes  : " << getMonitoredProcessCount());
    ZEROCP_LOG(Info, "  - Unmonitored processes: " << (m_processInfo.size() - getMonitoredProcessCount()));
    ZEROCP_LOG(Info, "==================================================");
}

} // namespace Runtime

} // namespace ZeroCP


