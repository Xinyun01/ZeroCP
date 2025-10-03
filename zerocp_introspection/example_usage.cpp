/**
 * @file example_usage.cpp
 * @brief Introspection 组件使用示例
 * 
 * 展示如何使用 Introspection 组件进行系统监控
 */

#include "introspection/introspection_server.hpp"
#include "introspection/introspection_client.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace zero_copy::introspection;

/**
 * @brief 格式化字节数
 */
std::string formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = bytes;
    
    while (size >= 1024 && unit_index < 4) {
        size /= 1024;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

/**
 * @brief 示例1: 基本的同步查询
 */
void example1_basic_query() {
    std::cout << "\n=== 示例1: 基本的同步查询 ===" << std::endl;

    // 1. 创建并配置服务端
    auto server = std::make_shared<IntrospectionServer>();
    IntrospectionConfig config;
    config.update_interval_ms = 1000;  // 1秒更新一次
    
    if (!server->start(config)) {
        std::cerr << "无法启动监控服务" << std::endl;
        return;
    }
    std::cout << "✓ 监控服务已启动" << std::endl;

    // 2. 创建并连接客户端
    IntrospectionClient client;
    if (!client.connectLocal(server)) {
        std::cerr << "无法连接到监控服务" << std::endl;
        return;
    }
    std::cout << "✓ 已连接到监控服务" << std::endl;

    // 等待数据收集
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 3. 查询系统指标
    SystemMetrics metrics;
    if (client.getMetrics(metrics)) {
        std::cout << "\n📊 系统指标:" << std::endl;
        std::cout << "  内存使用率: " << std::fixed << std::setprecision(1) 
                  << metrics.memory.memory_usage_percent << "%" << std::endl;
        std::cout << "  总内存: " << formatBytes(metrics.memory.total_memory) << std::endl;
        std::cout << "  已使用: " << formatBytes(metrics.memory.used_memory) << std::endl;
        std::cout << "  CPU使用率: " << metrics.load.cpu_usage_percent << "%" << std::endl;
        std::cout << "  进程数量: " << metrics.processes.size() << std::endl;
        std::cout << "  连接数量: " << metrics.connections.size() << std::endl;
    }

    // 4. 清理
    client.disconnect();
    server->stop();
    std::cout << "✓ 已清理资源" << std::endl;
}

/**
 * @brief 示例2: 异步事件订阅
 */
void example2_async_subscription() {
    std::cout << "\n=== 示例2: 异步事件订阅 ===" << std::endl;

    // 创建服务端
    auto server = std::make_shared<IntrospectionServer>();
    IntrospectionConfig config;
    config.update_interval_ms = 500;  // 500ms更新一次
    server->start(config);

    // 创建客户端
    IntrospectionClient client;
    client.connectLocal(server);

    std::cout << "✓ 开始订阅事件（运行5秒）..." << std::endl;

    // 订阅事件
    int event_count = 0;
    client.subscribe([&event_count](const IntrospectionEvent& event) {
        if (event.type == IntrospectionEventType::SYSTEM_UPDATE) {
            event_count++;
            std::cout << "\r事件 #" << event_count 
                      << " | 内存: " << std::fixed << std::setprecision(1)
                      << event.metrics.memory.memory_usage_percent << "% "
                      << "| CPU: " << event.metrics.load.cpu_usage_percent << "% "
                      << std::flush;
        } else if (event.type == IntrospectionEventType::ERROR) {
            std::cerr << "\n错误: " << event.error_message << std::endl;
        }
    });

    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << std::endl;

    // 取消订阅
    client.unsubscribe();
    std::cout << "✓ 已取消订阅，共收到 " << event_count << " 个事件" << std::endl;

    // 清理
    client.disconnect();
    server->stop();
}

/**
 * @brief 示例3: 进程监控与过滤
 */
void example3_process_monitoring() {
    std::cout << "\n=== 示例3: 进程监控与过滤 ===" << std::endl;

    // 创建服务端，只监控特定进程
    auto server = std::make_shared<IntrospectionServer>();
    IntrospectionConfig config;
    config.update_interval_ms = 1000;
    config.process_filter = {"bash", "systemd", "sshd"};  // 只监控这些进程
    server->start(config);

    // 创建客户端
    IntrospectionClient client;
    client.connectLocal(server);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 获取进程列表
    std::vector<ProcessInfo> processes;
    if (client.getProcessList(processes)) {
        std::cout << "\n📋 监控的进程（前10个）:" << std::endl;
        std::cout << std::left << std::setw(10) << "PID" 
                  << std::setw(20) << "名称" 
                  << std::setw(15) << "内存" 
                  << std::setw(10) << "线程数" 
                  << "状态" << std::endl;
        std::cout << std::string(65, '-') << std::endl;

        int count = 0;
        for (const auto& proc : processes) {
            if (count++ >= 10) break;
            std::cout << std::left << std::setw(10) << proc.pid
                      << std::setw(20) << proc.name
                      << std::setw(15) << formatBytes(proc.memory_usage)
                      << std::setw(10) << proc.threads_count
                      << proc.state << std::endl;
        }
        std::cout << "\n总共找到 " << processes.size() << " 个匹配的进程" << std::endl;
    }

    // 清理
    client.disconnect();
    server->stop();
}

/**
 * @brief 示例4: 动态配置更新
 */
void example4_dynamic_config() {
    std::cout << "\n=== 示例4: 动态配置更新 ===" << std::endl;

    // 创建服务端
    auto server = std::make_shared<IntrospectionServer>();
    IntrospectionConfig config;
    config.update_interval_ms = 1000;
    server->start(config);

    // 创建客户端
    IntrospectionClient client;
    client.connectLocal(server);

    // 获取当前配置
    IntrospectionConfig current_config;
    client.getConfig(current_config);
    std::cout << "初始更新间隔: " << current_config.update_interval_ms << "ms" << std::endl;

    // 更新配置
    IntrospectionConfig new_config;
    new_config.update_interval_ms = 500;  // 改为500ms
    new_config.process_filter = {"systemd"};  // 添加进程过滤
    
    if (client.requestConfigUpdate(new_config)) {
        std::cout << "✓ 配置已更新" << std::endl;
        
        // 验证配置
        client.getConfig(current_config);
        std::cout << "新的更新间隔: " << current_config.update_interval_ms << "ms" << std::endl;
        std::cout << "进程过滤器数量: " << current_config.process_filter.size() << std::endl;
    }

    // 清理
    client.disconnect();
    server->stop();
}

/**
 * @brief 示例5: 立即数据收集
 */
void example5_immediate_collection() {
    std::cout << "\n=== 示例5: 立即数据收集 ===" << std::endl;

    // 创建服务端（较长的更新间隔）
    auto server = std::make_shared<IntrospectionServer>();
    IntrospectionConfig config;
    config.update_interval_ms = 5000;  // 5秒更新一次
    server->start(config);

    // 创建客户端
    IntrospectionClient client;
    client.connectLocal(server);

    std::cout << "更新间隔设置为5秒，但我们可以立即请求数据..." << std::endl;

    // 立即收集数据（不等待定时更新）
    SystemMetrics metrics;
    if (client.requestCollectOnce(metrics)) {
        std::cout << "✓ 立即收集完成" << std::endl;
        std::cout << "  内存使用率: " << metrics.memory.memory_usage_percent << "%" << std::endl;
        std::cout << "  进程数量: " << metrics.processes.size() << std::endl;
    }

    // 清理
    client.disconnect();
    server->stop();
}

/**
 * @brief 示例6: 多客户端访问
 */
void example6_multiple_clients() {
    std::cout << "\n=== 示例6: 多客户端访问 ===" << std::endl;

    // 创建服务端
    auto server = std::make_shared<IntrospectionServer>();
    IntrospectionConfig config;
    config.update_interval_ms = 1000;
    server->start(config);

    std::cout << "✓ 服务端启动，支持多客户端连接" << std::endl;

    // 创建多个客户端
    IntrospectionClient client1, client2, client3;
    client1.connectLocal(server);
    client2.connectLocal(server);
    client3.connectLocal(server);

    std::cout << "✓ 已连接3个客户端" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 每个客户端独立查询
    SystemMetrics metrics1, metrics2, metrics3;
    client1.getMetrics(metrics1);
    client2.getMetrics(metrics2);
    client3.getMetrics(metrics3);

    std::cout << "客户端1 查询结果 - 内存: " << metrics1.memory.memory_usage_percent << "%" << std::endl;
    std::cout << "客户端2 查询结果 - 内存: " << metrics2.memory.memory_usage_percent << "%" << std::endl;
    std::cout << "客户端3 查询结果 - 内存: " << metrics3.memory.memory_usage_percent << "%" << std::endl;

    // 清理
    client1.disconnect();
    client2.disconnect();
    client3.disconnect();
    server->stop();
    std::cout << "✓ 所有客户端已断开" << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   Introspection 组件使用示例                    ║" << std::endl;
    std::cout << "║   Zero Copy Framework                         ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;

    try {
        // 运行所有示例
        example1_basic_query();
        example2_async_subscription();
        example3_process_monitoring();
        example4_dynamic_config();
        example5_immediate_collection();
        example6_multiple_clients();

        std::cout << "\n✓ 所有示例运行完成！" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}

