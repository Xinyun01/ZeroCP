#include <iostream>
#include <chrono>
#include "zerocp_daemon/diroute/diroute_memory_manager.hpp"

int main()
{
    std::cout << "=== 心跳诊断工具 ===" << std::endl;
    std::cout << std::endl;
    
    // 打开共享内存
    auto result = ZeroCP::Diroute::DirouteMemoryManager::openMemoryPool();
    if (!result)
    {
        std::cerr << "错误：无法打开共享内存。守护进程可能未运行。" << std::endl;
        return 1;
    }
    
    auto memoryManager = std::move(*result);
    auto& heartbeatPool = memoryManager.getHeartbeatPool();
    
    std::cout << "共享内存已打开" << std::endl;
    std::cout << "池容量: " << heartbeatPool.capacity() << std::endl;
    std::cout << std::endl;
    
    // 当前时间
    auto now = std::chrono::steady_clock::now();
    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    std::cout << "当前时间: " << now_ns << " ns" << std::endl;
    std::cout << std::endl;
    
    // 遍历所有槽位
    uint64_t offset = 0;
    std::cout << "槽位状态:" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;
    std::cout << "Offset | 心跳时间戳          | 年龄(秒)  | 状态" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;
    
    for (auto it = heartbeatPool.begin(); it != heartbeatPool.end(); ++it, ++offset)
    {
        uint64_t timestamp = it->load();
        
        if (timestamp == 0)
        {
            std::cout << offset << "      | 0                   | -         | 未使用" << std::endl;
        }
        else
        {
            uint64_t age_ns = now_ns - timestamp;
            double age_sec = age_ns / 1'000'000'000.0;
            
            std::string status;
            if (age_sec > 3.0)
            {
                status = "❌ 超时";
            }
            else if (age_sec > 1.0)
            {
                status = "⚠️  过时";
            }
            else
            {
                status = "✓ 正常";
            }
            
            std::cout << offset << "      | " << timestamp 
                     << " | " << age_sec 
                     << " | " << status << std::endl;
        }
    }
    
    std::cout << "--------------------------------------------------------" << std::endl;
    
    return 0;
}
