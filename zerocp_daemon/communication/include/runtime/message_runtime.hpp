#ifndef ZEROCP_MESSAGE_RUNTIME_HPP
#define ZEROCP_MESSAGE_RUNTIME_HPP
 
#include <string>
#include <cstdint>
#include <sys/types.h>
#include "zerocp_foundationLib/vocabulary/include/string.hpp"
namespace ZeroCP
{
using RuntimeName_t = ZeroCP::string<108>;
namespace Runtime
{

struct MessageStruct
{
    pid_t pid;
    uid_t uid;
    std::uint64_t timestampNs;
    RuntimeName_t appName;
};
class MessageRuntime
{
public:
    MessageRuntime(const RuntimeName_t& m_runtimeName) noexcept;
    pid_t get_pid();
    uid_t get_uid();
    std::uint64_t get_timestampNs();
    RuntimeName_t get_appName();
private:
    struct MessageStruct m_messageStruct;
};

} // namespace Runtime
} // namespace ZeroCP
#endif