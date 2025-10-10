/**
 * @file test_backend.cpp
 * @brief 测试 LogBackend 实现的示例程序
 * 
 * 编译方式：
 *   g++ -std=c++17 -I../include \
 *       test_backend.cpp \
 *       ../source/log_backend.cpp \
 *       ../source/logstream.cpp \
 *       -pthread -o test_backend
 * 
 * 运行：
 *   ./test_backend
 */

#include "log_backend.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace ZeroCP::Log;

// 测试 1: 基本功能测试
void test_basic_functionality()
{
    std::cout << "\n========== 测试 1: 基本功能 ==========\n";
    
    LogBackend backend;
    
    // 启动后台线程
    backend.start();
    std::cout << "✓ 后台线程已启动\n";
    
    // 提交几条日志
    backend.submitLog("[INFO] 第一条日志\n");
    backend.submitLog("[INFO] 第二条日志\n");
    backend.submitLog("[INFO] 第三条日志\n");
    std::cout << "✓ 已提交 3 条日志\n";
    
    // 等待处理
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 查看统计
    std::cout << "\n统计信息:\n";
    std::cout << "  已处理: " << backend.getProcessedCount() << " 条\n";
    std::cout << "  已丢弃: " << backend.getDroppedCount() << " 条\n";
    
    // 停止
    backend.stop();
    std::cout << "✓ 后台线程已停止\n";
}

// 测试 2: 高并发测试
void test_high_concurrency()
{
    std::cout << "\n========== 测试 2: 高并发 ==========\n";
    
    LogBackend backend;
    backend.start();
    
    const int NUM_MESSAGES = 1000;
    
    std::cout << "正在提交 " << NUM_MESSAGES << " 条日志...\n";
    
    // 快速提交大量日志
    for (int i = 0; i < NUM_MESSAGES; ++i) {
        std::string msg = "[TEST] 消息 #" + std::to_string(i) + "\n";
        backend.submitLog(msg);
    }
    
    std::cout << "✓ 已提交完成，等待处理...\n";
    
    // 等待处理
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 查看统计
    std::cout << "\n统计信息:\n";
    std::cout << "  已处理: " << backend.getProcessedCount() << " 条\n";
    std::cout << "  已丢弃: " << backend.getDroppedCount() << " 条\n";
    
    backend.stop();
}

// 测试 3: 停止时处理剩余消息
void test_remaining_messages()
{
    std::cout << "\n========== 测试 3: 剩余消息处理 ==========\n";
    
    LogBackend backend;
    backend.start();
    
    // 提交消息
    backend.submitLog("[INFO] 消息 1\n");
    backend.submitLog("[INFO] 消息 2\n");
    backend.submitLog("[INFO] 消息 3\n");
    
    std::cout << "✓ 已提交 3 条日志\n";
    std::cout << "立即停止（测试是否处理剩余消息）...\n";
    
    // 立即停止（不等待）
    backend.stop();
    
    // 查看统计
    std::cout << "\n统计信息:\n";
    std::cout << "  已处理: " << backend.getProcessedCount() << " 条\n";
    std::cout << "  (应该是 3，说明退出前处理了剩余消息)\n";
}

// 测试 4: 多线程提交
void test_multi_thread_submit()
{
    std::cout << "\n========== 测试 4: 多线程提交 ==========\n";
    
    LogBackend backend;
    backend.start();
    
    const int NUM_THREADS = 4;
    const int MESSAGES_PER_THREAD = 100;
    
    std::cout << "启动 " << NUM_THREADS << " 个线程，每个提交 " 
              << MESSAGES_PER_THREAD << " 条日志...\n";
    
    // 创建多个线程同时提交日志
    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&backend, t]() {
            for (int i = 0; i < MESSAGES_PER_THREAD; ++i) {
                std::string msg = "[线程" + std::to_string(t) + "] 消息 #" 
                                + std::to_string(i) + "\n";
                backend.submitLog(msg);
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "✓ 所有线程已完成提交\n";
    
    // 等待处理
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 查看统计
    std::cout << "\n统计信息:\n";
    std::cout << "  已处理: " << backend.getProcessedCount() << " 条\n";
    std::cout << "  已丢弃: " << backend.getDroppedCount() << " 条\n";
    std::cout << "  预期:   " << (NUM_THREADS * MESSAGES_PER_THREAD) << " 条\n";
    
    backend.stop();
}

// 测试 5: 性能测试
void test_performance()
{
    std::cout << "\n========== 测试 5: 性能测试 ==========\n";
    
    LogBackend backend;
    backend.start();
    
    const int NUM_MESSAGES = 10000;
    
    // 开始计时
    auto start = std::chrono::high_resolution_clock::now();
    
    // 提交大量日志
    for (int i = 0; i < NUM_MESSAGES; ++i) {
        backend.submitLog("[PERF] 性能测试消息\n");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    // 计算提交耗时
    auto submit_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "✓ 提交 " << NUM_MESSAGES << " 条日志耗时: " 
              << submit_duration.count() << " 微秒\n";
    std::cout << "  平均每条: " << (submit_duration.count() / NUM_MESSAGES) 
              << " 微秒\n";
    
    // 等待处理完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 查看统计
    std::cout << "\n统计信息:\n";
    std::cout << "  已处理: " << backend.getProcessedCount() << " 条\n";
    std::cout << "  已丢弃: " << backend.getDroppedCount() << " 条\n";
    
    backend.stop();
}

int main()
{
    std::cout << R"(
╔═══════════════════════════════════════════╗
║   LogBackend 测试程序                     ║
║   测试您的 workerThread() 实现           ║
╚═══════════════════════════════════════════╝
)";

    try {
        // 运行所有测试
        test_basic_functionality();
        test_high_concurrency();
        test_remaining_messages();
        test_multi_thread_submit();
        test_performance();
        
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "✅ 所有测试完成！\n";
        std::cout << std::string(50, '=') << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

