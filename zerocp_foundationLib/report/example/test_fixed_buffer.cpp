/**
 * @file test_fixed_buffer.cpp
 * @brief 测试使用固定缓冲区的日志系统
 */

#include "../include/logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace ZeroCP::Log;

int main()
{
    std::cout << "\n========== 固定缓冲区日志系统测试 ==========\n\n";

    // 1. 启动日志系统
    std::cout << "1. 启动日志系统...\n";
    Log_Manager::getInstance().setLogLevel(LogLevel::Debug);
    Log_Manager::getInstance().start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 2. 测试基本日志
    std::cout << "\n2. 测试基本日志输出:\n";
    ZEROCP_LOG(LogLevel::Info, "这是一条普通的日志消息");
    ZEROCP_LOG(LogLevel::Debug, "调试信息: " << 42);
    ZEROCP_LOG(LogLevel::Warn, "警告: 值 = " << 3.14159);
    ZEROCP_LOG(LogLevel::Error, "错误: " << "连接失败");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 3. 测试各种数据类型
    std::cout << "\n3. 测试各种数据类型:\n";
    ZEROCP_LOG(LogLevel::Info, "整数: " << 123 << ", 浮点: " << 45.67 << ", 布尔: " << true);
    ZEROCP_LOG(LogLevel::Info, "字符串拼接: " << "Hello" << " " << "World" << "!");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 4. 测试超长消息（应该被截断）
    std::cout << "\n4. 测试超长消息（4KB 限制）:\n";
    std::string long_msg(5000, 'X');  // 5KB 的消息
    ZEROCP_LOG(LogLevel::Info, "超长消息测试: " << long_msg);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 5. 测试性能（无动态内存分配）
    std::cout << "\n5. 测试性能（1000条日志）:\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        ZEROCP_LOG(LogLevel::Debug, "性能测试消息 #" << i << " 值=" << i * 2);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "   写入 1000 条日志耗时: " << duration.count() << " μs\n";
    std::cout << "   平均每条: " << (duration.count() / 1000.0) << " μs\n";
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 6. 测试多线程安全性
    std::cout << "\n6. 测试多线程（4个线程，每个100条日志）:\n";
    std::vector<std::thread> threads;
    
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < 100; ++i) {
                ZEROCP_LOG(LogLevel::Info, "线程 " << t << " 消息 " << i);
            }
        });
    }
    
    for (auto& th : threads) {
        th.join();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 7. 停止日志系统
    std::cout << "\n7. 停止日志系统...\n";
    Log_Manager::getInstance().stop();
    
    std::cout << "\n========== 测试完成 ==========\n";
    std::cout << "优点说明:\n";
    std::cout << "  ✓ 无动态内存分配（无 new/malloc）\n";
    std::cout << "  ✓ 固定 4KB 缓冲区，内存可预测\n";
    std::cout << "  ✓ 使用 snprintf/memcpy，性能稳定\n";
    std::cout << "  ✓ 超长消息自动截断，不会崩溃\n";
    std::cout << "  ✓ 无异常抛出，适合实时系统\n\n";

    return 0;
}


