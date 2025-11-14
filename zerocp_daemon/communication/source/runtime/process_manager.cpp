#include "zerocp_daemon/communication/include/runtime/process_runtime.hpp"

namespace ZeroCP
{

namespace Runtime
{
bool ProcessManager::registerProcess(   const RuntimeName_t& name,
                                        const uint32_t pid,
                                        const bool isMonitored )
{

    for(const auto& it : m_processInfo)
    {
        if (it.name == name)   // RuntimeName_t 支持 == 比较
        {
            ZEROCP_LOG(Error, "Failed to registerProcess because of name.");
            return false;      // 找到了
        }
    }
    m_processInfo.emplace_back(name, pid, isMonitored);
    return true;
}

bool ProcessManager::unregisterProcess(const RuntimeName_t& name)
{
    for(const auto& it : m_processInfo)
    {
        if (it->name == name)   // RuntimeName_t 支持 == 比较
        {
            m_processInfo.erase(it);  // 从列表中删除这一项
            return true;              // 删除成功
        }
    }
}

}

} // namespace ZeroCP


