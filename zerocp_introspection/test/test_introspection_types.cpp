/**
 * @file test_introspection_types.cpp
 * @brief 测试 introspection 数据类型
 */

#include <gtest/gtest.h>
#include "introspection/introspection_types.hpp"

using namespace zero_copy::introspection;

// 测试 MemoryInfo 结构
TEST(IntrospectionTypesTest, MemoryInfoBasic) {
    MemoryInfo mem;
    mem.total_memory = 16ULL * 1024 * 1024 * 1024;  // 16 GB
    mem.available_memory = 8ULL * 1024 * 1024 * 1024;  // 8 GB
    mem.used_memory = mem.total_memory - mem.available_memory;
    mem.memory_usage_percent = (mem.used_memory * 100.0) / mem.total_memory;

    EXPECT_EQ(mem.total_memory, 16ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(mem.available_memory, 8ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(mem.used_memory, 8ULL * 1024 * 1024 * 1024);
    EXPECT_DOUBLE_EQ(mem.memory_usage_percent, 50.0);
}

// 测试 ProcessInfo 结构
TEST(IntrospectionTypesTest, ProcessInfoBasic) {
    ProcessInfo proc;
    proc.pid = 1234;
    proc.name = "test_process";
    proc.state = "R";
    proc.threads_count = 4;
    proc.memory_usage = 1024 * 1024;  // 1 MB

    EXPECT_EQ(proc.pid, 1234);
    EXPECT_EQ(proc.name, "test_process");
    EXPECT_EQ(proc.state, "R");
    EXPECT_EQ(proc.threads_count, 4);
    EXPECT_EQ(proc.memory_usage, 1024 * 1024);
}

// 测试 ConnectionInfo 结构
TEST(IntrospectionTypesTest, ConnectionInfoBasic) {
    ConnectionInfo conn;
    conn.local_address = "127.0.0.1";
    conn.local_port = 8080;
    conn.remote_address = "192.168.1.100";
    conn.remote_port = 12345;
    conn.state = "ESTABLISHED";
    conn.pid = 5678;
    conn.process_name = "nginx";

    EXPECT_EQ(conn.local_address, "127.0.0.1");
    EXPECT_EQ(conn.local_port, 8080);
    EXPECT_EQ(conn.remote_address, "192.168.1.100");
    EXPECT_EQ(conn.remote_port, 12345);
    EXPECT_EQ(conn.state, "ESTABLISHED");
    EXPECT_EQ(conn.pid, 5678);
    EXPECT_EQ(conn.process_name, "nginx");
}

// 测试 LoadInfo 结构
TEST(IntrospectionTypesTest, LoadInfoBasic) {
    LoadInfo load;
    load.load_average_1min = 1.5;
    load.load_average_5min = 1.2;
    load.load_average_15min = 0.9;
    load.cpu_usage_percent = 45.5;

    EXPECT_DOUBLE_EQ(load.load_average_1min, 1.5);
    EXPECT_DOUBLE_EQ(load.load_average_5min, 1.2);
    EXPECT_DOUBLE_EQ(load.load_average_15min, 0.9);
    EXPECT_DOUBLE_EQ(load.cpu_usage_percent, 45.5);
}

// 测试 SystemMetrics 结构
TEST(IntrospectionTypesTest, SystemMetricsBasic) {
    SystemMetrics metrics;
    
    metrics.memory.total_memory = 8ULL * 1024 * 1024 * 1024;
    metrics.memory.available_memory = 4ULL * 1024 * 1024 * 1024;
    
    ProcessInfo proc1;
    proc1.pid = 100;
    proc1.name = "process1";
    
    ProcessInfo proc2;
    proc2.pid = 200;
    proc2.name = "process2";
    
    metrics.processes.push_back(proc1);
    metrics.processes.push_back(proc2);
    
    metrics.load.cpu_usage_percent = 30.0;

    EXPECT_EQ(metrics.memory.total_memory, 8ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(metrics.processes.size(), 2);
    EXPECT_EQ(metrics.processes[0].pid, 100);
    EXPECT_EQ(metrics.processes[1].pid, 200);
    EXPECT_DOUBLE_EQ(metrics.load.cpu_usage_percent, 30.0);
}

// 测试 IntrospectionConfig 结构
TEST(IntrospectionTypesTest, ConfigBasic) {
    IntrospectionConfig config;
    
    config.update_interval_ms = 1000;
    config.process_filter = {"nginx", "systemd"};
    config.connection_filter = {80, 443};

    EXPECT_EQ(config.update_interval_ms, 1000);
    EXPECT_EQ(config.process_filter.size(), 2);
    EXPECT_EQ(config.process_filter[0], "nginx");
    EXPECT_EQ(config.connection_filter.size(), 2);
    EXPECT_EQ(config.connection_filter[0], 80);
}

// 测试 IntrospectionEvent 结构
TEST(IntrospectionTypesTest, EventBasic) {
    IntrospectionEvent event;
    
    event.type = IntrospectionEventType::SYSTEM_UPDATE;
    event.timestamp = std::chrono::system_clock::now();
    event.metrics.memory.total_memory = 1024;

    EXPECT_EQ(event.type, IntrospectionEventType::SYSTEM_UPDATE);
    EXPECT_EQ(event.metrics.memory.total_memory, 1024);
}

// 测试事件类型枚举
TEST(IntrospectionTypesTest, EventTypes) {
    EXPECT_EQ(static_cast<int>(IntrospectionEventType::SYSTEM_UPDATE), 0);
    EXPECT_EQ(static_cast<int>(IntrospectionEventType::CONFIG_CHANGE), 1);
    EXPECT_EQ(static_cast<int>(IntrospectionEventType::ERROR), 2);
}

// 测试状态枚举
TEST(IntrospectionTypesTest, StateTypes) {
    EXPECT_EQ(static_cast<int>(IntrospectionState::STOPPED), 0);
    EXPECT_EQ(static_cast<int>(IntrospectionState::RUNNING), 1);
    EXPECT_EQ(static_cast<int>(IntrospectionState::ERROR), 2);
}

// 测试默认构造函数
TEST(IntrospectionTypesTest, DefaultConstruction) {
    MemoryInfo mem;
    EXPECT_EQ(mem.total_memory, 0);
    EXPECT_EQ(mem.available_memory, 0);
    EXPECT_EQ(mem.used_memory, 0);
    EXPECT_DOUBLE_EQ(mem.memory_usage_percent, 0.0);

    ProcessInfo proc;
    EXPECT_EQ(proc.pid, 0);
    EXPECT_TRUE(proc.name.empty());
    EXPECT_TRUE(proc.state.empty());

    ConnectionInfo conn;
    EXPECT_EQ(conn.local_port, 0);
    EXPECT_EQ(conn.remote_port, 0);
    EXPECT_EQ(conn.pid, 0);

    LoadInfo load;
    EXPECT_DOUBLE_EQ(load.load_average_1min, 0.0);
    EXPECT_DOUBLE_EQ(load.cpu_usage_percent, 0.0);
}

// 测试拷贝和赋值
TEST(IntrospectionTypesTest, CopyAndAssignment) {
    SystemMetrics metrics1;
    metrics1.memory.total_memory = 1024;
    
    ProcessInfo proc;
    proc.pid = 123;
    metrics1.processes.push_back(proc);

    // 拷贝构造
    SystemMetrics metrics2 = metrics1;
    EXPECT_EQ(metrics2.memory.total_memory, 1024);
    EXPECT_EQ(metrics2.processes.size(), 1);
    EXPECT_EQ(metrics2.processes[0].pid, 123);

    // 赋值
    SystemMetrics metrics3;
    metrics3 = metrics1;
    EXPECT_EQ(metrics3.memory.total_memory, 1024);
    EXPECT_EQ(metrics3.processes.size(), 1);
}

// 测试大数值处理
TEST(IntrospectionTypesTest, LargeValues) {
    MemoryInfo mem;
    mem.total_memory = 1ULL << 40;  // 1 TB
    
    EXPECT_GT(mem.total_memory, 0);
    EXPECT_EQ(mem.total_memory, 1ULL << 40);
}

