#ifndef ZEROCP_DEFAULT_MEMORYPOOL_HPP
#define ZEROCP_DEFAULT_MEMORYPOOL_HPP

#include "zerocp_daemon/memory/include/heartbeat_pool.hpp"

namespace zerocp::memory
{

struct DefaultMemoryPool
{
    HeartbeatPool heartbeat_pool{};
};

} // namespace zerocp::memory

#endif // ZEROCP_DEFAULT_MEMORYPOOL_HPP
