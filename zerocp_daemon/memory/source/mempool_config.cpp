#include "mempool_config.hpp"
namespace ZeroCP
{

namespace Memory_Pool
{
const MemPool_Config::Pool_Entries& MemPool_Config::getMemPoolConfigInfo() const noexcept
{
    return pool_entries;
}

//添加内存池
void MemPool_Config::addPoolEntry(Pool_Entry entry) noexcept
{
    if(pool_entries.size() >= pool_entries.capacity())
    {
        pool_entries.push_back(entry);
    }
    else{
        printf("Memory_Pool: pool_entries is full\n");
    }
    
}
//默认配置
MemPool_Config& MemPool_Config::Default_Config() noexcept
{
    pool_entries.push_back({128, 10000});
    pool_entries.push_back({1024, 5000});
    pool_entries.push_back({1024 * 16, 1000});
    pool_entries.push_back({1024 * 128, 200});
    pool_entries.push_back({1024 * 512, 50});
    pool_entries.push_back({1024 * 1024, 30});
    pool_entries.push_back({1024 * 1024 * 4, 10});
    return *this;
}



}
}