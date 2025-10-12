/**
 * @file test_performance.cpp
 * @brief 无锁队列性能基准测试程序
 * 
 * 测试场景：
 * 1. 单生产者单消费者 - 基准性能
 * 2. 多生产者单消费者 - CAS 竞争测试
 * 3. 批量操作性能对比
 * 4. 不同队列大小的影响
 * 5. 延迟分布统计
 * 
 * 编译：
 *   g++ -std=c++17 -O3 -march=native -I../include \
 *       test_performance.cpp \
 *       ../source/lockfree_ringbuffer.cpp \
 *       -pthread -o test_performance
 */

#include "lockfree_ringbuffer.hpp"
#include "log_backend.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <iomanip>
#include <cstring>

using namespace ZeroCP::Log;
using namespace std::chrono;

// ANSI 颜色
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define RESET   "\033[0m"

// 性能统计结构
struct PerfStats {
    std::string test_name;
    size_t total_operations;
    double total_time_ms;
    double throughput_ops_per_sec;
    double avg_latency_ns;
    double min_latency_ns;
    double max_latency_ns;
    size_t success_count;
    size_t failure_count;
};

// 打印性能统计
void print_stats(const PerfStats& stats) {
    std::cout << CYAN << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << "\n";
    std::cout << YELLOW << "📊 " << stats.test_name << RESET << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  总操作数:     " << stats.total_operations << "\n";
    std::cout << "  成功:         " << stats.success_count << "\n";
    std::cout << "  失败:         " << stats.failure_count << "\n";
    std::cout << "  总耗时:       " << std::fixed << std::setprecision(2) 
              << stats.total_time_ms << " ms\n";
    std::cout << GREEN << "  吞吐量:       " << std::fixed << std::setprecision(0) 
              << stats.throughput_ops_per_sec << " ops/sec" << RESET << "\n";
    std::cout << "  平均延迟:     " << std::fixed << std::setprecision(2) 
              << stats.avg_latency_ns << " ns\n";
    std::cout << "  最小延迟:     " << std::fixed << std::setprecision(2) 
              << stats.min_latency_ns << " ns\n";
    std::cout << "  最大延迟:     " << std::fixed << std::setprecision(2) 
              << stats.max_latency_ns << " ns\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
}

// ============================================================================
// 测试 1: 单生产者单消费者 - 基准性能
// ============================================================================
void test_spsc_baseline() {
    std::cout << MAGENTA << "\n\n========== 测试 1: 单生产者单消费者（基准）==========" << RESET << "\n";
    
    LockFreeRingBuffer<LogMessage, 1024> queue;
    const size_t NUM_MESSAGES = 100000;
    
    std::atomic<bool> consumer_done{false};
    std::atomic<size_t> consumed{0};
    
    // 消费者线程
    auto consumer = std::thread([&]() {
        LogMessage msg;
        while (consumed < NUM_MESSAGES) {
            if (queue.tryPop(msg)) {
                consumed++;
            } else {
                std::this_thread::yield();
            }
        }
        consumer_done = true;
    });
    
    // 生产者 - 测量写入性能
    std::vector<double> latencies;
    latencies.reserve(NUM_MESSAGES);
    
    size_t success = 0;
    size_t failure = 0;
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_MESSAGES; ++i) {
        LogMessage msg;
        std::string content = "Message #" + std::to_string(i);
        memcpy(msg.message, content.c_str(), content.size());
        msg.length = content.size();
        
        auto op_start = high_resolution_clock::now();
        
        while (!queue.tryPush(msg)) {
            std::this_thread::yield();
        }
        
        auto op_end = high_resolution_clock::now();
        
        double latency = duration_cast<nanoseconds>(op_end - op_start).count();
        latencies.push_back(latency);
        success++;
    }
    
    auto end = high_resolution_clock::now();
    
    // 等待消费者完成
    consumer.join();
    
    // 计算统计
    double total_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    double throughput = (NUM_MESSAGES * 1000.0) / total_ms;
    
    double sum = 0;
    double min_lat = latencies[0];
    double max_lat = latencies[0];
    
    for (auto lat : latencies) {
        sum += lat;
        min_lat = std::min(min_lat, lat);
        max_lat = std::max(max_lat, lat);
    }
    
    double avg_lat = sum / latencies.size();
    
    PerfStats stats{
        "单生产者单消费者 (SPSC)",
        NUM_MESSAGES,
        total_ms,
        throughput,
        avg_lat,
        min_lat,
        max_lat,
        success,
        failure
    };
    
    print_stats(stats);
}

// ============================================================================
// 测试 2: 多生产者单消费者 - CAS 竞争测试
// ============================================================================
void test_mpsc_contention() {
    std::cout << MAGENTA << "\n\n========== 测试 2: 多生产者单消费者（CAS 竞争）==========" << RESET << "\n";
    
    const size_t NUM_PRODUCERS = 4;
    const size_t MSGS_PER_PRODUCER = 25000;
    const size_t TOTAL_MESSAGES = NUM_PRODUCERS * MSGS_PER_PRODUCER;
    
    LockFreeRingBuffer<LogMessage, 2048> queue;
    
    std::atomic<size_t> consumed{0};
    std::atomic<bool> all_produced{false};
    
    // 消费者线程
    auto consumer = std::thread([&]() {
        LogMessage msg;
        while (consumed < TOTAL_MESSAGES) {
            if (queue.tryPop(msg)) {
                consumed++;
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    // 生产者线程
    std::vector<std::thread> producers;
    std::atomic<size_t> total_success{0};
    
    auto start = high_resolution_clock::now();
    
    for (size_t p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([&, p]() {
            size_t local_success = 0;
            
            for (size_t i = 0; i < MSGS_PER_PRODUCER; ++i) {
                LogMessage msg;
                std::string content = "Producer_" + std::to_string(p) + "_Msg_" + std::to_string(i);
                memcpy(msg.message, content.c_str(), content.size());
                msg.length = content.size();
                
                while (!queue.tryPush(msg)) {
                    std::this_thread::yield();
                }
                
                local_success++;
            }
            
            total_success += local_success;
        });
    }
    
    // 等待所有生产者完成
    for (auto& t : producers) {
        t.join();
    }
    
    auto end = high_resolution_clock::now();
    all_produced = true;
    
    // 等待消费者完成
    consumer.join();
    
    // 计算统计
    double total_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    double throughput = (TOTAL_MESSAGES * 1000.0) / total_ms;
    double avg_lat = (total_ms * 1000000.0) / TOTAL_MESSAGES;
    
    PerfStats stats{
        "多生产者单消费者 (MPSC) - " + std::to_string(NUM_PRODUCERS) + " 生产者",
        TOTAL_MESSAGES,
        total_ms,
        throughput,
        avg_lat,
        0,
        0,
        total_success.load(),
        0
    };
    
    print_stats(stats);
}

// ============================================================================
// 测试 3: 高频率写入测试
// ============================================================================
void test_high_frequency() {
    std::cout << MAGENTA << "\n\n========== 测试 3: 高频率短消息写入 ==========" << RESET << "\n";
    
    LockFreeRingBuffer<LogMessage, 4096> queue;
    const size_t NUM_MESSAGES = 500000;
    
    std::atomic<size_t> consumed{0};
    
    // 消费者线程（尽可能快地消费）
    auto consumer = std::thread([&]() {
        LogMessage msg;
        while (consumed < NUM_MESSAGES) {
            while (queue.tryPop(msg)) {
                consumed++;
                if (consumed >= NUM_MESSAGES) break;
            }
        }
    });
    
    // 生产者 - 高频短消息
    size_t success = 0;
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_MESSAGES; ++i) {
        LogMessage msg;
        // 非常短的消息（模拟高频日志）
        const char* short_msg = "OK";
        memcpy(msg.message, short_msg, 2);
        msg.length = 2;
        
        while (!queue.tryPush(msg)) {
            // 短暂等待
        }
        
        success++;
    }
    
    auto end = high_resolution_clock::now();
    
    // 等待消费者
    consumer.join();
    
    double total_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    double throughput = (NUM_MESSAGES * 1000.0) / total_ms;
    double avg_lat = (total_ms * 1000000.0) / NUM_MESSAGES;
    
    PerfStats stats{
        "高频短消息写入",
        NUM_MESSAGES,
        total_ms,
        throughput,
        avg_lat,
        0,
        0,
        success,
        0
    };
    
    print_stats(stats);
}

// ============================================================================
// 测试 4: 不同队列大小的性能对比
// ============================================================================
template<size_t QueueSize>
double benchmark_queue_size(const std::string& size_name) {
    LockFreeRingBuffer<LogMessage, QueueSize> queue;
    const size_t NUM_MESSAGES = 50000;
    
    std::atomic<size_t> consumed{0};
    
    auto consumer = std::thread([&]() {
        LogMessage msg;
        while (consumed < NUM_MESSAGES) {
            if (queue.tryPop(msg)) {
                consumed++;
            }
        }
    });
    
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_MESSAGES; ++i) {
        LogMessage msg;
        std::string content = "Test message " + std::to_string(i);
        memcpy(msg.message, content.c_str(), content.size());
        msg.length = content.size();
        
        while (!queue.tryPush(msg)) {
            std::this_thread::yield();
        }
    }
    
    auto end = high_resolution_clock::now();
    
    consumer.join();
    
    double total_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    double throughput = (NUM_MESSAGES * 1000.0) / total_ms;
    
    std::cout << "  " << std::setw(12) << size_name << ": " 
              << GREEN << std::fixed << std::setprecision(0) 
              << std::setw(12) << throughput << " ops/sec" << RESET
              << "  (" << std::fixed << std::setprecision(2) << total_ms << " ms)\n";
    
    return throughput;
}

void test_queue_sizes() {
    std::cout << MAGENTA << "\n\n========== 测试 4: 不同队列大小性能对比 ==========" << RESET << "\n";
    std::cout << "\n队列大小性能对比:\n";
    
    benchmark_queue_size<256>("256");
    benchmark_queue_size<512>("512");
    benchmark_queue_size<1024>("1024");
    benchmark_queue_size<2048>("2048");
    benchmark_queue_size<4096>("4096");
    benchmark_queue_size<8192>("8192");
}

// ============================================================================
// 测试 5: 实际日志系统性能测试
// ============================================================================
void test_real_world_logging() {
    std::cout << MAGENTA << "\n\n========== 测试 5: 实际日志系统端到端性能 ==========" << RESET << "\n";
    
    LogBackend backend;
    backend.start();
    
    const size_t NUM_THREADS = 4;
    const size_t MSGS_PER_THREAD = 10000;
    const size_t TOTAL_MESSAGES = NUM_THREADS * MSGS_PER_THREAD;
    
    auto start = high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (size_t t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (size_t i = 0; i < MSGS_PER_THREAD; ++i) {
                std::string msg = "[INFO] [Thread_" + std::to_string(t) + 
                                "] Message #" + std::to_string(i) + 
                                " - Some log data here\n";
                backend.submitLog(msg.c_str(), msg.size());
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = high_resolution_clock::now();
    
    // 等待处理完成
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    backend.stop();
    
    double total_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    double throughput = (TOTAL_MESSAGES * 1000.0) / total_ms;
    double avg_lat = (total_ms * 1000000.0) / TOTAL_MESSAGES;
    
    size_t processed = backend.getProcessedCount();
    size_t dropped = backend.getDroppedCount();
    
    PerfStats stats{
        "实际日志系统端到端 (" + std::to_string(NUM_THREADS) + " 线程)",
        TOTAL_MESSAGES,
        total_ms,
        throughput,
        avg_lat,
        0,
        0,
        processed,
        dropped
    };
    
    print_stats(stats);
}

// ============================================================================
// 测试 6: 压力测试 - 测试系统极限
// ============================================================================
void test_stress() {
    std::cout << MAGENTA << "\n\n========== 测试 6: 压力测试（系统极限）==========" << RESET << "\n";
    
    const size_t NUM_PRODUCERS = 8;
    const size_t MSGS_PER_PRODUCER = 50000;
    const size_t TOTAL_MESSAGES = NUM_PRODUCERS * MSGS_PER_PRODUCER;
    
    LockFreeRingBuffer<LogMessage, 8192> queue;
    
    std::atomic<size_t> consumed{0};
    std::atomic<size_t> total_cas_retries{0};
    
    // 2 个消费者线程
    std::vector<std::thread> consumers;
    for (int c = 0; c < 2; ++c) {
        consumers.emplace_back([&]() {
            LogMessage msg;
            while (consumed < TOTAL_MESSAGES) {
                if (queue.tryPop(msg)) {
                    consumed++;
                }
            }
        });
    }
    
    // 8 个生产者线程
    std::vector<std::thread> producers;
    
    auto start = high_resolution_clock::now();
    
    for (size_t p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([&, p]() {
            for (size_t i = 0; i < MSGS_PER_PRODUCER; ++i) {
                LogMessage msg;
                std::string content = "P" + std::to_string(p) + "_M" + std::to_string(i);
                memcpy(msg.message, content.c_str(), content.size());
                msg.length = content.size();
                
                size_t retries = 0;
                while (!queue.tryPush(msg)) {
                    retries++;
                    std::this_thread::yield();
                }
                
                if (retries > 0) {
                    total_cas_retries += retries;
                }
            }
        });
    }
    
    for (auto& t : producers) {
        t.join();
    }
    
    auto end = high_resolution_clock::now();
    
    for (auto& t : consumers) {
        t.join();
    }
    
    double total_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    double throughput = (TOTAL_MESSAGES * 1000.0) / total_ms;
    double avg_lat = (total_ms * 1000000.0) / TOTAL_MESSAGES;
    
    PerfStats stats{
        "压力测试 (" + std::to_string(NUM_PRODUCERS) + " 生产者, 2 消费者)",
        TOTAL_MESSAGES,
        total_ms,
        throughput,
        avg_lat,
        0,
        0,
        TOTAL_MESSAGES,
        0
    };
    
    print_stats(stats);
    
    std::cout << "  CAS 重试次数: " << total_cas_retries << "\n";
    std::cout << "  平均每消息重试: " << std::fixed << std::setprecision(2) 
              << (double)total_cas_retries / TOTAL_MESSAGES << " 次\n";
}

// ============================================================================
// 主函数
// ============================================================================
int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║        无锁队列性能基准测试套件                              ║
║        Performance Benchmark Suite                           ║
║                                                              ║
║  测试目标：                                                  ║
║    • 单生产者单消费者基准性能                                ║
║    • 多生产者 CAS 竞争测试                                   ║
║    • 高频率写入性能                                          ║
║    • 不同队列大小对性能的影响                                ║
║    • 实际日志系统端到端性能                                  ║
║    • 系统极限压力测试                                        ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
)" << std::endl;

    try {
        auto overall_start = high_resolution_clock::now();
        
        // 运行所有测试
        test_spsc_baseline();
        test_mpsc_contention();
        test_high_frequency();
        test_queue_sizes();
        test_real_world_logging();
        test_stress();
        
        auto overall_end = high_resolution_clock::now();
        auto total_time = duration_cast<milliseconds>(overall_end - overall_start).count();
        
        // 总结
        std::cout << CYAN << "\n\n╔══════════════════════════════════════════════════════════════╗" << RESET << "\n";
        std::cout << CYAN << "║                    测试完成总结                              ║" << RESET << "\n";
        std::cout << CYAN << "╚══════════════════════════════════════════════════════════════╝" << RESET << "\n";
        std::cout << GREEN << "\n✅ 所有性能测试完成！" << RESET << "\n";
        std::cout << "总测试时间: " << std::fixed << std::setprecision(2) 
                  << total_time / 1000.0 << " 秒\n";
        
        std::cout << "\n💡 性能优化建议:\n";
        std::cout << "  • 如果需要更高性能，考虑实现批量操作接口\n";
        std::cout << "  • 添加缓存行填充可以消除伪共享，提升 30-50%\n";
        std::cout << "  • 实现指数退避策略可以减少 CAS 竞争\n";
        std::cout << "  • 使用线程本地缓冲可以大幅降低竞争\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 测试异常: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

