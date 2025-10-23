#ifndef ZEROCP_DAEMON_MEMORYCONFIG_HPP
#define ZEROCP_DAEMON_MEMORYCONFIG_HPP

namespace ZeroCP
{

namespace Daemon
{

class MemoryConfig
{

public:
    MemoryConfig() = default;
    ~MemoryConfig() = default;
    MemoryConfig(const MemoryConfig&) = delete;
    MemoryConfig(MemoryConfig&&) noexcept = delete;
    MemoryConfig& operator=(const MemoryConfig&) = delete;
    MemoryConfig& operator=(MemoryConfig&&) noexcept = delete;



}


}

}

#endif