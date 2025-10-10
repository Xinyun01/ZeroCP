#ifndef MEMPOOL_CONFIG_HPP
#define MEMPOOL_CONFIG_HPP

namespace ZeroCP
{

namespace Memory_Pool
{
struct MemPool_Config{
// 配置单条目的内存配置
    struct Pool_Entry
    {
        Pool_Entry(uint64_t size, uint64_t count) noexcept
        : pool_size(size)
        , pool_count(count) 
        {
        }
        uint64_t pool_size = 0;
        uint64_t pool_count = 0;

    };

    //容器包含所有的条目,用于配置所有的内存池
    using Pool_Entries = ZeroCP::Static_Vector<Pool_Entry, 16>;

    Pool_Entries pool_entries;

    MemPool_Config() noexcept = default;

    /// @brief 获取内存池配置的函数
    /// @return 包含配置信息的大小和块的数量
    const Pool_Entries& getMemPoolConfigInfo() const noexcept;


    /// @brief 添加内存池配置条目
    /// @param entry 内存池配置条目
    void addPoolEntry(Pool_Entry entry) noexcept;

    MemPool_Config& Default_Config() noexcept;

}

}



}

#endif