#include "runtime/message_runtime.hpp"
#include <unistd.h>        // getpid, getuid
#include <time.h>          // clock_gettime

namespace ZeroCP
{
namespace Runtime
{

static std::uint64_t nowNs() noexcept
{
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<std::uint64_t>(ts.tv_nsec);
}

MessageRuntime::MessageRuntime(const RuntimeName_t& m_runtimeName) noexcept
{
    m_messageStruct.pid = ::getpid();
    m_messageStruct.uid = ::getuid();
    m_messageStruct.timestampNs = nowNs();
    m_messageStruct.appName = m_runtimeName;
}

pid_t MessageRuntime::get_pid()
{
    return m_messageStruct.pid;
}

uid_t MessageRuntime::get_uid()
{
    return m_messageStruct.uid;
}

std::uint64_t MessageRuntime::get_timestampNs()
{
    return m_messageStruct.timestampNs;
}

RuntimeName_t MessageRuntime::get_appName()
{
    return m_messageStruct.appName;
}

} // namespace Runtime
} // namespace ZeroCP