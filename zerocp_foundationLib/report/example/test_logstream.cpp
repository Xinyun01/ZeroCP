#include "../include/logging.hpp"
#include "../include/logsteam.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace ZeroCP::Log;

int main() {
    std::cout << "=== LogStream 队列测试 ===" << std::endl;
    
    // 1. 设置日志级别
    Log_Manager::getInstance().setLogLevel(LogLevel::Debug);
    
    // 2. 测试各种类型的日志消息
    std::cout << "发送日志消息到队列..." << std::endl;
    
    // 使用宏方式
    ZEROCP_LOG(LogLevel::Info, "这是一条信息日志");
    ZEROCP_LOG(LogLevel::Warn, "警告: 内存使用率过高 " << 85.6 << "%");
    ZEROCP_LOG(LogLevel::Error, "错误代码: " << 404 << " 文件未找到");
    ZEROCP_LOG(LogLevel::Debug, "调试信息: 变量值 = " << 42 << ", 布尔值 = " << true);
    
    // 直接使用 LogStream 方式
    LogStream(__FILE__, __LINE__, __FUNCTION__, LogLevel::Info) 
        << "直接使用 LogStream: 用户ID=" << 12345 << ", 状态=" << "active";
    
    // 3. 等待一段时间让后台线程处理
    std::cout << "等待后台线程处理日志..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 4. 测试多线程场景
    std::cout << "测试多线程日志..." << std::endl;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 3; ++j) {
                ZEROCP_LOG(LogLevel::Info, 
                    "线程 " << i << " 消息 " << j << " 时间戳: " << 
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count());
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 5. 等待后台线程处理完所有消息
    std::cout << "等待所有日志处理完成..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    std::cout << "测试完成！" << std::endl;
    return 0;
}
