#ifndef ZEROCP_PROCESSINFO_HPP
#define ZEROCP_PROCESSINFO_HPP
namespace ZeroCP
{
using RuntimeName_t = ZeroCP::string<108>;
namespace Runtime
{

struct ProcessInfo
{
    RuntimeName_t name;   // 逻辑名
    uint32_t      pid;    // 进程 pid
    bool          isMonitored; // 是否监控

        ProcessInfo(const RuntimeName_t& n,
                uint32_t p,
                bool mon) noexcept
        : name(n)
        , pid(p)
        , isMonitored(mon)
        {
        }
};
}
}


#endif 