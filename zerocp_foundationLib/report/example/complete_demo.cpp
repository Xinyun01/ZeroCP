/**
 * @file complete_demo.cpp
 * @brief 完整的异步日志系统使用示例
 * 
 * 这个示例展示了如何使用 ZEROCP_LOG 宏记录带时间戳的日志
 * 
 * 编译方式：
 *   g++ -std=c++17 -I../include \
 *       complete_demo.cpp \
 *       ../source/log_backend.cpp \
 *       ../source/logstream.cpp \
 *       ../source/logging.cpp \
 *       -pthread -o complete_demo
 * 
 * 运行：
 *   ./complete_demo
 */

#include "logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace ZeroCP::Log;

// 示例 1: 基本日志输出
void example_basic_logging()
{
    std::cout << "\n========== 示例 1: 基本日志输出 ==========\n";
    
    // 使用 ZEROCP_LOG 宏记录不同级别的日志
    ZEROCP_LOG(LogLevel::Debug, "这是一条调试日志");
    ZEROCP_LOG(LogLevel::Info, "这是一条信息日志");
    ZEROCP_LOG(LogLevel::Warn, "这是一条警告日志");
    ZEROCP_LOG(LogLevel::Error, "这是一条错误日志");
    ZEROCP_LOG(LogLevel::Fatal, "这是一条致命错误日志");
    
    // 等待日志处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 示例 2: 流式输出多种类型
void example_stream_types()
{
    std::cout << "\n========== 示例 2: 流式输出多种类型 ==========\n";
    
    int number = 42;
    double pi = 3.14159;
    bool flag = true;
    std::string name = "ZeroCopy";
    
    // 可以像 std::cout 一样使用 << 操作符
    ZEROCP_LOG(LogLevel::Info, "整数: " << number);
    ZEROCP_LOG(LogLevel::Info, "浮点数: " << pi);
    ZEROCP_LOG(LogLevel::Info, "布尔值: " << flag);
    ZEROCP_LOG(LogLevel::Info, "字符串: " << name);
    
    // 混合类型
    ZEROCP_LOG(LogLevel::Info, "混合输出: " << name << " 的数字是 " << number << ", π≈" << pi);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 示例 3: 日志级别过滤
void example_log_level_filtering()
{
    std::cout << "\n========== 示例 3: 日志级别过滤 ==========\n";
    
    // 设置日志级别为 Warn，低于此级别的日志不会输出
    Log_Manager::getInstance().setLogLevel(LogLevel::Warn);
    std::cout << "设置日志级别为 Warn\n";
    
    ZEROCP_LOG(LogLevel::Debug, "这条 DEBUG 不会显示");
    ZEROCP_LOG(LogLevel::Info, "这条 INFO 不会显示");
    ZEROCP_LOG(LogLevel::Warn, "这条 WARN 会显示");
    ZEROCP_LOG(LogLevel::Error, "这条 ERROR 会显示");
    
    // 恢复为 Info 级别
    Log_Manager::getInstance().setLogLevel(LogLevel::Info);
    std::cout << "恢复日志级别为 Info\n";
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 示例 4: 多线程日志记录
void worker_thread(int thread_id, int message_count)
{
    for (int i = 0; i < message_count; ++i) {
        ZEROCP_LOG(LogLevel::Info, "线程 " << thread_id << " 发送消息 #" << i);
        // 稍微休眠一下，模拟实际工作
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void example_multithreaded_logging()
{
    std::cout << "\n========== 示例 4: 多线程日志记录 ==========\n";
    
    const int NUM_THREADS = 4;
    const int MESSAGES_PER_THREAD = 5;
    
    std::vector<std::thread> threads;
    
    ZEROCP_LOG(LogLevel::Info, "启动 " << NUM_THREADS << " 个线程，每个发送 " << MESSAGES_PER_THREAD << " 条消息");
    
    // 创建多个线程同时记录日志
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker_thread, i, MESSAGES_PER_THREAD);
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    ZEROCP_LOG(LogLevel::Info, "所有线程已完成");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

// 示例 5: 性能测试
void example_performance_test()
{
    std::cout << "\n========== 示例 5: 性能测试 ==========\n";
    
    const int MESSAGE_COUNT = 10000;
    
    ZEROCP_LOG(LogLevel::Info, "开始性能测试：发送 " << MESSAGE_COUNT << " 条日志");
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 快速发送大量日志
    for (int i = 0; i < MESSAGE_COUNT; ++i) {
        ZEROCP_LOG(LogLevel::Debug, "性能测试消息 #" << i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "提交 " << MESSAGE_COUNT << " 条日志耗时: " 
              << duration.count() << " 微秒\n";
    std::cout << "平均每条: " << (duration.count() / MESSAGE_COUNT) << " 微秒\n";
    
    ZEROCP_LOG(LogLevel::Info, "性能测试完成");
    
    // 等待处理完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 显示统计信息
    auto& backend = Log_Manager::getInstance().getBackend();
    std::cout << "\n统计信息:\n";
    std::cout << "  已处理: " << backend.getProcessedCount() << " 条\n";
    std::cout << "  已丢弃: " << backend.getDroppedCount() << " 条\n";
}

// 示例 6: 实际应用场景
void simulate_application()
{
    std::cout << "\n========== 示例 6: 模拟实际应用 ==========\n";
    
    ZEROCP_LOG(LogLevel::Info, "应用程序启动");
    
    // 模拟初始化
    ZEROCP_LOG(LogLevel::Debug, "正在初始化配置...");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ZEROCP_LOG(LogLevel::Info, "配置初始化成功");
    
    // 模拟数据库连接
    ZEROCP_LOG(LogLevel::Debug, "正在连接数据库...");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ZEROCP_LOG(LogLevel::Info, "数据库连接成功");
    
    // 模拟处理请求
    for (int i = 1; i <= 5; ++i) {
        ZEROCP_LOG(LogLevel::Info, "处理请求 #" << i);
        
        if (i == 3) {
            // 模拟警告
            ZEROCP_LOG(LogLevel::Warn, "请求 #" << i << " 响应时间较长: 1500ms");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // 模拟错误
    ZEROCP_LOG(LogLevel::Error, "处理请求时发生错误: 连接超时");
    
    // 模拟清理
    ZEROCP_LOG(LogLevel::Debug, "正在清理资源...");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ZEROCP_LOG(LogLevel::Info, "应用程序正常退出");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

int main()
{
    std::cout << R"(
╔═══════════════════════════════════════════════════════════╗
║   ZeroCopy 异步日志系统 - 完整示例                        ║
║   展示带时间戳的多级别日志输出                            ║
╚═══════════════════════════════════════════════════════════╝
)" << std::endl;

    // 日志系统会在 Log_Manager 构造时自动启动
    std::cout << "✓ 日志系统已自动启动\n" << std::endl;
    
    try {
        // 运行所有示例
        example_basic_logging();
        example_stream_types();
        example_log_level_filtering();
        example_multithreaded_logging();
        example_performance_test();
        simulate_application();
        
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "✅ 所有示例运行完成！\n";
        
        // 最终统计
        auto& backend = Log_Manager::getInstance().getBackend();
        std::cout << "\n最终统计:\n";
        std::cout << "  总共处理: " << backend.getProcessedCount() << " 条日志\n";
        std::cout << "  丢弃: " << backend.getDroppedCount() << " 条日志\n";
        std::cout << std::string(60, '=') << "\n";
        
    } catch (const std::exception& e) {
        ZEROCP_LOG(LogLevel::Fatal, "程序异常: " << e.what());
        return 1;
    }
    
    // 日志系统会在 Log_Manager 析构时自动停止
    std::cout << "\n✓ 日志系统即将自动停止...\n";
    
    return 0;
}


