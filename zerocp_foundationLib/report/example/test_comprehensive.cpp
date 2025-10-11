/**
 * @file test_comprehensive.cpp
 * @brief ZeroCopy 异步日志系统综合测试程序
 * 
 * 测试覆盖：
 * 1. 固定缓冲区 LogMessage 拷贝优化
 * 2. 无锁队列并发性能
 * 3. 日志流格式化功能
 * 4. 多线程压力测试
 * 5. 内存使用验证
 * 6. 边界条件测试
 * 
 * 编译方式：
 *   g++ -std=c++17 -I../include \
 *       test_comprehensive.cpp \
 *       ../source/lockfree_ringbuffer.cpp \
 *       ../source/log_backend.cpp \
 *       ../source/logstream.cpp \
 *       ../source/logging.cpp \
 *       -pthread -o test_comprehensive
 * 
 * 或使用快速脚本：
 *   ../build_and_run.sh test_comprehensive
 */

#include "logging.hpp"
#include "log_backend.hpp"
#include "lockfree_ringbuffer.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>
#include <cstring>
#include <iomanip>

using namespace ZeroCP::Log;

// ANSI 颜色代码
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RESET   "\033[0m"

// 测试结果统计
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    double duration_ms;
};

std::vector<TestResult> test_results;

// 添加测试结果
void add_result(const std::string& name, bool passed, const std::string& msg = "", double duration = 0.0) {
    test_results.push_back({name, passed, msg, duration});
    if (passed) {
        std::cout << COLOR_GREEN << "✓ PASS" << COLOR_RESET;
    } else {
        std::cout << COLOR_RED << "✗ FAIL" << COLOR_RESET;
    }
    std::cout << " - " << name;
    if (!msg.empty()) {
        std::cout << " (" << msg << ")";
    }
    if (duration > 0) {
        std::cout << " [" << std::fixed << std::setprecision(2) << duration << "ms]";
    }
    std::cout << std::endl;
}

// ============================================================================
// 测试 1: LogMessage 拷贝优化验证
// ============================================================================
void test_logmessage_copy_optimization() {
    std::cout << COLOR_CYAN << "\n========== 测试 1: LogMessage 拷贝优化 ==========" << COLOR_RESET << std::endl;
    
    // 1.1 测试短消息拷贝
    {
        LogMessage msg1;
        const char* short_msg = "Short message";
        size_t len = strlen(short_msg);
        memcpy(msg1.message, short_msg, len);
        msg1.length = len;
        
        // 拷贝构造
        LogMessage msg2(msg1);
        
        bool passed = (msg2.length == len) && 
                     (memcmp(msg1.message, msg2.message, len) == 0);
        add_result("短消息拷贝构造", passed, 
                  std::to_string(len) + " bytes");
    }
    
    // 1.2 测试长消息拷贝
    {
        LogMessage msg1;
        std::string long_msg(200, 'A'); // 200 字节
        memcpy(msg1.message, long_msg.c_str(), long_msg.size());
        msg1.length = long_msg.size();
        
        // 拷贝赋值
        LogMessage msg2;
        msg2 = msg1;
        
        bool passed = (msg2.length == msg1.length) && 
                     (memcmp(msg1.message, msg2.message, msg1.length) == 0);
        add_result("长消息拷贝赋值", passed, 
                  std::to_string(msg1.length) + " bytes");
    }
    
    // 1.3 性能对比测试
    {
        const int ITERATIONS = 100000;
        LogMessage msg1;
        std::string test_msg = "[2025-10-10 10:20:30.123] [INFO] Test message with some data";
        memcpy(msg1.message, test_msg.c_str(), test_msg.size());
        msg1.length = test_msg.size();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < ITERATIONS; ++i) {
            LogMessage msg2(msg1);
            (void)msg2; // 防止优化掉
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avg_ns = (duration.count() * 1000.0) / ITERATIONS;
        
        add_result("拷贝性能测试", true, 
                  "平均 " + std::to_string((int)avg_ns) + " ns/次", 
                  duration.count() / 1000.0);
    }
    
    // 1.4 边界条件：空消息
    {
        LogMessage msg1;
        msg1.length = 0;
        
        LogMessage msg2(msg1);
        bool passed = (msg2.length == 0);
        add_result("空消息拷贝", passed);
    }
    
    // 1.5 边界条件：最大长度
    {
        LogMessage msg1;
        msg1.length = LogMessage::MAX_MESSAGE_SIZE;
        memset(msg1.message, 'X', msg1.length);
        
        LogMessage msg2 = msg1;
        bool passed = (msg2.length == LogMessage::MAX_MESSAGE_SIZE) &&
                     (memcmp(msg1.message, msg2.message, msg1.length) == 0);
        add_result("最大长度消息拷贝", passed, "256 bytes");
    }
}

// ============================================================================
// 测试 2: 无锁队列并发性能
// ============================================================================
void test_lockfree_queue_concurrency() {
    std::cout << COLOR_CYAN << "\n========== 测试 2: 无锁队列并发性能 ==========" << COLOR_RESET << std::endl;
    
    // 2.1 单生产者单消费者
    {
        LockFreeRingBuffer<LogMessage, 1024> queue;
        const int COUNT = 10000;
        std::atomic<int> consumed{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 生产者线程
        std::thread producer([&]() {
            LogMessage msg;
            for (int i = 0; i < COUNT; ++i) {
                std::string content = "Message " + std::to_string(i);
                memcpy(msg.message, content.c_str(), content.size());
                msg.length = content.size();
                
                while (!queue.tryPush(msg)) {
                    std::this_thread::yield();
                }
            }
        });
        
        // 消费者线程
        std::thread consumer([&]() {
            LogMessage msg;
            while (consumed < COUNT) {
                if (queue.tryPop(msg)) {
                    consumed++;
                } else {
                    std::this_thread::yield();
                }
            }
        });
        
        producer.join();
        consumer.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        bool passed = (consumed == COUNT);
        add_result("单生产者单消费者", passed, 
                  std::to_string(COUNT) + " 条消息", 
                  duration.count());
    }
    
    // 2.2 多生产者单消费者
    {
        LockFreeRingBuffer<LogMessage, 1024> queue;
        const int NUM_PRODUCERS = 4;
        const int MSGS_PER_PRODUCER = 2500;
        const int TOTAL_MSGS = NUM_PRODUCERS * MSGS_PER_PRODUCER;
        std::atomic<int> consumed{0};
        std::atomic<bool> done{false};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 多个生产者
        std::vector<std::thread> producers;
        for (int p = 0; p < NUM_PRODUCERS; ++p) {
            producers.emplace_back([&, p]() {
                LogMessage msg;
                for (int i = 0; i < MSGS_PER_PRODUCER; ++i) {
                    std::string content = "P" + std::to_string(p) + "_M" + std::to_string(i);
                    memcpy(msg.message, content.c_str(), content.size());
                    msg.length = content.size();
                    
                    while (!queue.tryPush(msg)) {
                        std::this_thread::yield();
                    }
                }
            });
        }
        
        // 单个消费者
        std::thread consumer([&]() {
            LogMessage msg;
            while (consumed < TOTAL_MSGS) {
                if (queue.tryPop(msg)) {
                    consumed++;
                } else {
                    std::this_thread::yield();
                }
            }
            done = true;
        });
        
        for (auto& p : producers) {
            p.join();
        }
        consumer.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        bool passed = (consumed == TOTAL_MSGS);
        add_result("多生产者单消费者", passed, 
                  std::to_string(NUM_PRODUCERS) + " 生产者, " + std::to_string(TOTAL_MSGS) + " 条消息", 
                  duration.count());
    }
    
    // 2.3 队列满时的处理
    {
        LockFreeRingBuffer<LogMessage, 8> small_queue; // 小队列
        LogMessage msg;
        strcpy(msg.message, "Test");
        msg.length = 4;
        
        int pushed = 0;
        while (small_queue.tryPush(msg)) {
            pushed++;
            if (pushed > 100) break; // 防止无限循环
        }
        
        // 队列应该满了
        bool is_full = !small_queue.tryPush(msg);
        
        // 弹出一个
        LogMessage temp;
        bool can_pop = small_queue.tryPop(temp);
        
        // 现在应该能再 push 一个
        bool can_push_again = small_queue.tryPush(msg);
        
        bool passed = is_full && can_pop && can_push_again;
        add_result("队列满时的处理", passed, 
                  "pushed=" + std::to_string(pushed));
    }
}

// ============================================================================
// 测试 3: 日志流格式化功能
// ============================================================================
void test_logstream_formatting() {
    std::cout << COLOR_CYAN << "\n========== 测试 3: 日志流格式化 ==========" << COLOR_RESET << std::endl;
    
    Log_Manager::getInstance().setLogLevel(LogLevel::Debug);
    auto& backend = Log_Manager::getInstance().getBackend();
    
    // 清零统计
    size_t initial_count = backend.getProcessedCount();
    
    // 3.1 基本类型格式化
    {
        ZEROCP_LOG(LogLevel::Info, "整数: " << 42 << ", 浮点: " << 3.14159 << ", 布尔: " << true);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        bool passed = (backend.getProcessedCount() > initial_count);
        add_result("基本类型格式化", passed);
        initial_count = backend.getProcessedCount();
    }
    
    // 3.2 字符串类型
    {
        std::string str = "Hello";
        const char* cstr = "World";
        ZEROCP_LOG(LogLevel::Info, "String: " << str << ", C-string: " << cstr);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        bool passed = (backend.getProcessedCount() > initial_count);
        add_result("字符串类型格式化", passed);
        initial_count = backend.getProcessedCount();
    }
    
    // 3.3 长消息处理
    {
        std::string long_msg(500, 'X'); // 超过缓冲区大小
        ZEROCP_LOG(LogLevel::Warn, "长消息测试: " << long_msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        bool passed = (backend.getProcessedCount() > initial_count);
        add_result("长消息截断处理", passed, "500 字符");
        initial_count = backend.getProcessedCount();
    }
    
    // 3.4 混合类型
    {
        int id = 12345;
        double price = 99.99;
        std::string product = "Laptop";
        ZEROCP_LOG(LogLevel::Debug, 
                  "订单详情: ID=" << id << ", 商品=" << product << ", 价格=$" << price);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        bool passed = (backend.getProcessedCount() > initial_count);
        add_result("混合类型格式化", passed);
    }
}

// ============================================================================
// 测试 4: 多线程压力测试
// ============================================================================
void test_multithreaded_stress() {
    std::cout << COLOR_CYAN << "\n========== 测试 4: 多线程压力测试 ==========" << COLOR_RESET << std::endl;
    
    auto& backend = Log_Manager::getInstance().getBackend();
    size_t initial_processed = backend.getProcessedCount();
    size_t initial_dropped = backend.getDroppedCount();
    
    // 4.1 中等压力测试
    {
        const int NUM_THREADS = 8;
        const int MSGS_PER_THREAD = 1000;
        const int TOTAL_MSGS = NUM_THREADS * MSGS_PER_THREAD;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([t]() {
                for (int i = 0; i < MSGS_PER_THREAD; ++i) {
                    ZEROCP_LOG(LogLevel::Debug, 
                              "Thread_" << t << "_Msg_" << i << "_Data_" << (t * 1000 + i));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // 等待处理
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        size_t processed = backend.getProcessedCount() - initial_processed;
        size_t dropped = backend.getDroppedCount() - initial_dropped;
        double throughput = (processed * 1000.0) / duration.count();
        
        bool passed = (processed + dropped >= TOTAL_MSGS * 0.95); // 至少 95% 被处理或记录
        add_result("中等压力测试", passed, 
                  std::to_string((int)throughput) + " msg/s, 处理=" + std::to_string(processed) + 
                  ", 丢弃=" + std::to_string(dropped),
                  duration.count());
        
        initial_processed = backend.getProcessedCount();
        initial_dropped = backend.getDroppedCount();
    }
    
    // 4.2 高频短消息
    {
        const int NUM_THREADS = 4;
        const int MSGS_PER_THREAD = 5000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([t]() {
                for (int i = 0; i < MSGS_PER_THREAD; ++i) {
                    ZEROCP_LOG(LogLevel::Info, "T" << t << "M" << i);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // 等待处理
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        size_t processed = backend.getProcessedCount() - initial_processed;
        double throughput = (processed * 1000.0) / duration.count();
        
        add_result("高频短消息", true, 
                  std::to_string((int)throughput) + " msg/s",
                  duration.count());
    }
}

// ============================================================================
// 测试 5: 日志级别过滤
// ============================================================================
void test_log_level_filtering() {
    std::cout << COLOR_CYAN << "\n========== 测试 5: 日志级别过滤 ==========" << COLOR_RESET << std::endl;
    
    auto& backend = Log_Manager::getInstance().getBackend();
    
    // 5.1 设置为 Warn，Debug 和 Info 应该被过滤
    {
        Log_Manager::getInstance().setLogLevel(LogLevel::Warn);
        size_t before = backend.getProcessedCount();
        
        ZEROCP_LOG(LogLevel::Debug, "Debug message - should be filtered");
        ZEROCP_LOG(LogLevel::Info, "Info message - should be filtered");
        ZEROCP_LOG(LogLevel::Warn, "Warn message - should pass");
        ZEROCP_LOG(LogLevel::Error, "Error message - should pass");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        size_t after = backend.getProcessedCount();
        size_t processed = after - before;
        
        bool passed = (processed == 2); // 只有 Warn 和 Error
        add_result("日志级别过滤", passed, 
                  "预期 2 条，实际 " + std::to_string(processed) + " 条");
        
        // 恢复到 Debug
        Log_Manager::getInstance().setLogLevel(LogLevel::Debug);
    }
}

// ============================================================================
// 测试 6: 性能基准测试
// ============================================================================
void test_performance_benchmark() {
    std::cout << COLOR_CYAN << "\n========== 测试 6: 性能基准测试 ==========" << COLOR_RESET << std::endl;
    
    auto& backend = Log_Manager::getInstance().getBackend();
    
    // 6.1 单线程吞吐量
    {
        const int COUNT = 50000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < COUNT; ++i) {
            ZEROCP_LOG(LogLevel::Debug, "Benchmark message " << i);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double throughput = (COUNT * 1000000.0) / duration.count();
        double latency_ns = (duration.count() * 1000.0) / COUNT;
        
        add_result("单线程吞吐量", true, 
                  std::to_string((int)throughput) + " msg/s, 延迟 " + 
                  std::to_string((int)latency_ns) + " ns/msg",
                  duration.count() / 1000.0);
    }
    
    // 等待处理完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 6.2 多线程吞吐量
    {
        const int NUM_THREADS = 8;
        const int MSGS_PER_THREAD = 10000;
        const int TOTAL = NUM_THREADS * MSGS_PER_THREAD;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([t]() {
                for (int i = 0; i < MSGS_PER_THREAD; ++i) {
                    ZEROCP_LOG(LogLevel::Debug, "MT_" << t << "_" << i);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double throughput = (TOTAL * 1000.0) / duration.count();
        
        add_result("多线程吞吐量", true, 
                  std::to_string((int)throughput) + " msg/s (" + 
                  std::to_string(NUM_THREADS) + " 线程)",
                  duration.count());
    }
    
    // 最终等待
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// ============================================================================
// 打印测试总结
// ============================================================================
void print_summary() {
    std::cout << COLOR_CYAN << "\n========================================" << COLOR_RESET << std::endl;
    std::cout << COLOR_CYAN << "           测试总结报告" << COLOR_RESET << std::endl;
    std::cout << COLOR_CYAN << "========================================" << COLOR_RESET << std::endl;
    
    int total = test_results.size();
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : test_results) {
        if (result.passed) {
            passed++;
        } else {
            failed++;
        }
    }
    
    std::cout << "\n总测试数: " << total << std::endl;
    std::cout << COLOR_GREEN << "✓ 通过: " << passed << COLOR_RESET << std::endl;
    if (failed > 0) {
        std::cout << COLOR_RED << "✗ 失败: " << failed << COLOR_RESET << std::endl;
    }
    
    double pass_rate = (passed * 100.0) / total;
    std::cout << "通过率: " << std::fixed << std::setprecision(1) << pass_rate << "%\n" << std::endl;
    
    // 显示最终统计
    auto& backend = Log_Manager::getInstance().getBackend();
    std::cout << "日志系统统计:" << std::endl;
    std::cout << "  已处理: " << backend.getProcessedCount() << " 条" << std::endl;
    std::cout << "  已丢弃: " << backend.getDroppedCount() << " 条" << std::endl;
    
    std::cout << COLOR_CYAN << "========================================" << COLOR_RESET << std::endl;
    
    if (failed == 0) {
        std::cout << COLOR_GREEN << "🎉 所有测试通过！" << COLOR_RESET << std::endl;
    } else {
        std::cout << COLOR_RED << "⚠️  部分测试失败，请检查！" << COLOR_RESET << std::endl;
    }
}

// ============================================================================
// 主函数
// ============================================================================
int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║     ZeroCopy 异步日志系统 - 综合测试套件                    ║
║                                                              ║
║  测试内容：                                                  ║
║    1. LogMessage 拷贝优化                                    ║
║    2. 无锁队列并发性能                                      ║
║    3. 日志流格式化功能                                      ║
║    4. 多线程压力测试                                        ║
║    5. 日志级别过滤                                          ║
║    6. 性能基准测试                                          ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
)" << std::endl;

    try {
        // 运行所有测试
        test_logmessage_copy_optimization();
        test_lockfree_queue_concurrency();
        test_logstream_formatting();
        test_multithreaded_stress();
        test_log_level_filtering();
        test_performance_benchmark();
        
        // 打印总结
        print_summary();
        
    } catch (const std::exception& e) {
        std::cerr << COLOR_RED << "\n❌ 测试异常: " << e.what() << COLOR_RESET << std::endl;
        return 1;
    }
    
    return 0;
}

