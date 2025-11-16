#ifndef ZEROCP_PROCESSINFO_HPP
#define ZEROCP_PROCESSINFO_HPP

#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include <cstring>

namespace ZeroCP
{
using RuntimeName_t = ZeroCP::string<108>;
namespace Runtime
{

struct ProcessInfo
{
    RuntimeName_t name;        // 逻辑名
    uint32_t      pid;         // 进程 pid
    bool          isMonitored; // 是否监控

    ProcessInfo(const RuntimeName_t& n,
                uint32_t p,
                bool mon) noexcept
        : name(n)
        , pid(p)
        , isMonitored(mon)
    {
    }
    
    // 比较运算符：按名称比较
    bool operator==(const ProcessInfo& other) const noexcept
    {
        return std::strcmp(name.c_str(), other.name.c_str()) == 0;
    }
    
    bool operator!=(const ProcessInfo& other) const noexcept
    {
        return !(*this == other);
    }
    
    // 按名称比较
    bool hasName(const RuntimeName_t& n) const noexcept
    {
        return std::strcmp(name.c_str(), n.c_str()) == 0;
    }
    
    // 按PID比较
    bool hasPid(uint32_t p) const noexcept
    {
        return pid == p;
    }
    
    // 检查进程是否被监控
    bool isBeingMonitored() const noexcept
    {
        return isMonitored;
    }
    
    // 设置监控状态
    void setMonitored(bool monitored) noexcept
    {
        isMonitored = monitored;
    }
};
}
}


#endif 
