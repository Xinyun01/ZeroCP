#include "logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace ZeroCP::Log;

int main()
{
    std::cout << "=== 测试日志系统启动流程 ===" << std::endl;
    
    // 步骤1: 第一次调用日志宏
    std::cout << "\n[主线程] 第一次调用 ZEROCP_LOG..." << std::endl;
    ZEROCP_LOG(LogLevel::Info, "这是第一条日志，会触发单例创建和后台线程启动");
    
    // 此时：
    // - Log_Manager 单例已创建
    // - LogBackend 已创建
    // - 后台线程已启动，正在运行 workerThread()
    
    std::cout << "[主线程] 等待 100ms，让后台线程处理..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 步骤2: 发送更多日志
    std::cout << "\n[主线程] 发送多条日志..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        ZEROCP_LOG(LogLevel::Info, "日志消息 #" << i);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // 步骤3: 查看统计信息
    std::cout << "\n[主线程] 统计信息:" << std::endl;
    auto& backend = Log_Manager::getInstance().getBackend();
    std::cout << "已处理: " << backend.getProcessedCount() << " 条" << std::endl;
    std::cout << "已丢弃: " << backend.getDroppedCount() << " 条" << std::endl;
    
    // 步骤4: 测试高负载（队列容量测试）
    std::cout << "\n[主线程] 测试高负载（发送 2000 条日志）..." << std::endl;
    for (int i = 0; i < 2000; ++i) {
        ZEROCP_LOG(LogLevel::Debug, "压力测试消息 #" << i);
    }
    
    std::cout << "[主线程] 等待处理..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "\n[主线程] 最终统计:" << std::endl;
    std::cout << "已处理: " << backend.getProcessedCount() << " 条" << std::endl;
    std::cout << "已丢弃: " << backend.getDroppedCount() << " 条" << std::endl;
    
    std::cout << "\n[主线程] 程序即将退出，~Log_Manager() 会调用 stop() 停止后台线程" << std::endl;
    
    return 0;
}

// 输出示例：
// === 测试日志系统启动流程 ===
//
// [主线程] 第一次调用 ZEROCP_LOG...
// [后台线程] 启动完成，开始处理日志
// [2025-10-10 14:30:15] [INFO] [test_startup.cpp:12] [main] 这是第一条日志...
//
// [主线程] 发送多条日志...
// [2025-10-10 14:30:15] [INFO] [test_startup.cpp:22] [main] 日志消息 #0
// [2025-10-10 14:30:15] [INFO] [test_startup.cpp:22] [main] 日志消息 #1
// ...

