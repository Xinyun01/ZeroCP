/**
 * @file test_introspection_client.cpp
 * @brief 测试 IntrospectionClient 类
 */

#include <gtest/gtest.h>
#include "introspection/introspection_client.hpp"
#include "introspection/introspection_server.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace zero_copy::introspection;

class IntrospectionClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_shared<IntrospectionServer>();
        client = std::make_unique<IntrospectionClient>();
        
        // 启动服务器
        IntrospectionConfig config;
        config.update_interval_ms = 500;
        server->start(config);
    }

    void TearDown() override {
        if (client && client->isConnected()) {
            client->disconnect();
        }
        if (server && server->getState() == IntrospectionState::RUNNING) {
            server->stop();
        }
        client.reset();
        server.reset();
    }

    std::shared_ptr<IntrospectionServer> server;
    std::unique_ptr<IntrospectionClient> client;
};

// 测试连接和断开
TEST_F(IntrospectionClientTest, ConnectAndDisconnect) {
    EXPECT_FALSE(client->isConnected());

    EXPECT_TRUE(client->connectLocal(server));
    EXPECT_TRUE(client->isConnected());

    client->disconnect();
    EXPECT_FALSE(client->isConnected());
}

// 测试重复连接
TEST_F(IntrospectionClientTest, DoubleConnect) {
    EXPECT_TRUE(client->connectLocal(server));
    EXPECT_TRUE(client->isConnected());

    // 第二次连接应该失败
    EXPECT_FALSE(client->connectLocal(server));

    client->disconnect();
}

// 测试重复断开
TEST_F(IntrospectionClientTest, DoubleDisconnect) {
    EXPECT_TRUE(client->connectLocal(server));
    
    client->disconnect();
    EXPECT_FALSE(client->isConnected());

    // 第二次断开不应该崩溃
    client->disconnect();
    EXPECT_FALSE(client->isConnected());
}

// 测试连接到空服务器
TEST_F(IntrospectionClientTest, ConnectToNullServer) {
    EXPECT_FALSE(client->connectLocal(nullptr));
    EXPECT_FALSE(client->isConnected());
}

// 测试获取完整指标
TEST_F(IntrospectionClientTest, GetMetrics) {
    EXPECT_TRUE(client->connectLocal(server));

    // 等待数据收集
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    SystemMetrics metrics;
    EXPECT_TRUE(client->getMetrics(metrics));

    // 验证数据
    EXPECT_GT(metrics.memory.total_memory, 0);
    EXPECT_GE(metrics.memory.memory_usage_percent, 0.0);
    EXPECT_LE(metrics.memory.memory_usage_percent, 100.0);

    client->disconnect();
}

// 测试未连接时获取指标
TEST_F(IntrospectionClientTest, GetMetricsNotConnected) {
    SystemMetrics metrics;
    EXPECT_FALSE(client->getMetrics(metrics));
}

// 测试获取内存信息
TEST_F(IntrospectionClientTest, GetMemoryInfo) {
    EXPECT_TRUE(client->connectLocal(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    MemoryInfo memory;
    EXPECT_TRUE(client->getMemoryInfo(memory));

    EXPECT_GT(memory.total_memory, 0);
    EXPECT_GT(memory.available_memory, 0);
    EXPECT_LE(memory.available_memory, memory.total_memory);
    EXPECT_GE(memory.memory_usage_percent, 0.0);
    EXPECT_LE(memory.memory_usage_percent, 100.0);

    client->disconnect();
}

// 测试获取进程列表
TEST_F(IntrospectionClientTest, GetProcessList) {
    EXPECT_TRUE(client->connectLocal(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::vector<ProcessInfo> processes;
    EXPECT_TRUE(client->getProcessList(processes));

    // 应该至少有一些进程
    EXPECT_GT(processes.size(), 0);

    // 验证进程数据
    for (const auto& proc : processes) {
        EXPECT_GT(proc.pid, 0);
        EXPECT_FALSE(proc.name.empty());
    }

    client->disconnect();
}

// 测试获取连接列表
TEST_F(IntrospectionClientTest, GetConnectionList) {
    EXPECT_TRUE(client->connectLocal(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::vector<ConnectionInfo> connections;
    EXPECT_TRUE(client->getConnectionList(connections));

    // 连接列表可能为空或非空，都是合法的
    // 只验证数据格式
    for (const auto& conn : connections) {
        EXPECT_GE(conn.local_port, 0);
        EXPECT_GE(conn.remote_port, 0);
    }

    client->disconnect();
}

// 测试获取负载信息
TEST_F(IntrospectionClientTest, GetLoadInfo) {
    EXPECT_TRUE(client->connectLocal(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    LoadInfo load;
    EXPECT_TRUE(client->getLoadInfo(load));

    EXPECT_GE(load.load_average_1min, 0.0);
    EXPECT_GE(load.load_average_5min, 0.0);
    EXPECT_GE(load.load_average_15min, 0.0);
    EXPECT_GE(load.cpu_usage_percent, 0.0);
    EXPECT_LE(load.cpu_usage_percent, 100.0);

    client->disconnect();
}

// 测试获取配置
TEST_F(IntrospectionClientTest, GetConfig) {
    EXPECT_TRUE(client->connectLocal(server));

    IntrospectionConfig config;
    EXPECT_TRUE(client->getConfig(config));

    EXPECT_EQ(config.update_interval_ms, 500);

    client->disconnect();
}

// 测试请求配置更新
TEST_F(IntrospectionClientTest, RequestConfigUpdate) {
    EXPECT_TRUE(client->connectLocal(server));

    IntrospectionConfig new_config;
    new_config.update_interval_ms = 1000;
    new_config.process_filter = {"test_process"};

    EXPECT_TRUE(client->requestConfigUpdate(new_config));

    // 验证配置已更新
    IntrospectionConfig retrieved;
    EXPECT_TRUE(client->getConfig(retrieved));
    EXPECT_EQ(retrieved.update_interval_ms, 1000);
    EXPECT_EQ(retrieved.process_filter.size(), 1);

    client->disconnect();
}

// 测试未连接时请求配置更新
TEST_F(IntrospectionClientTest, RequestConfigUpdateNotConnected) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;
    
    EXPECT_FALSE(client->requestConfigUpdate(config));
}

// 测试立即收集
TEST_F(IntrospectionClientTest, RequestCollectOnce) {
    EXPECT_TRUE(client->connectLocal(server));

    SystemMetrics metrics;
    EXPECT_TRUE(client->requestCollectOnce(metrics));

    // 验证数据
    EXPECT_GT(metrics.memory.total_memory, 0);

    client->disconnect();
}

// 测试订阅和取消订阅
TEST_F(IntrospectionClientTest, SubscribeAndUnsubscribe) {
    EXPECT_TRUE(client->connectLocal(server));

    std::atomic<int> event_count{0};

    auto callback = [&event_count](const IntrospectionEvent& event) {
        if (event.type == IntrospectionEventType::SYSTEM_UPDATE) {
            event_count++;
        }
    };

    EXPECT_TRUE(client->subscribe(callback));

    // 等待一些事件
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 应该收到至少 2 次事件
    EXPECT_GE(event_count.load(), 2);

    client->unsubscribe();

    int count_before = event_count.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 取消订阅后不应该再收到事件
    EXPECT_EQ(event_count.load(), count_before);

    client->disconnect();
}

// 测试重复订阅
TEST_F(IntrospectionClientTest, DoubleSubscribe) {
    EXPECT_TRUE(client->connectLocal(server));

    std::atomic<int> count1{0};
    std::atomic<int> count2{0};

    auto callback1 = [&count1](const IntrospectionEvent&) { count1++; };
    auto callback2 = [&count2](const IntrospectionEvent&) { count2++; };

    EXPECT_TRUE(client->subscribe(callback1));
    
    // 第二次订阅应该失败（每个客户端只能有一个订阅）
    EXPECT_FALSE(client->subscribe(callback2));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 只有第一个回调应该被调用
    EXPECT_GT(count1.load(), 0);
    EXPECT_EQ(count2.load(), 0);

    client->disconnect();
}

// 测试未连接时订阅
TEST_F(IntrospectionClientTest, SubscribeNotConnected) {
    auto callback = [](const IntrospectionEvent&) {};
    
    EXPECT_FALSE(client->subscribe(callback));
}

// 测试多客户端同时访问
TEST_F(IntrospectionClientTest, MultipleClients) {
    auto client1 = std::make_unique<IntrospectionClient>();
    auto client2 = std::make_unique<IntrospectionClient>();
    auto client3 = std::make_unique<IntrospectionClient>();

    EXPECT_TRUE(client1->connectLocal(server));
    EXPECT_TRUE(client2->connectLocal(server));
    EXPECT_TRUE(client3->connectLocal(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 每个客户端独立查询
    SystemMetrics metrics1, metrics2, metrics3;
    EXPECT_TRUE(client1->getMetrics(metrics1));
    EXPECT_TRUE(client2->getMetrics(metrics2));
    EXPECT_TRUE(client3->getMetrics(metrics3));

    // 数据应该相同（来自同一个服务器）
    EXPECT_EQ(metrics1.memory.total_memory, metrics2.memory.total_memory);
    EXPECT_EQ(metrics2.memory.total_memory, metrics3.memory.total_memory);

    client1->disconnect();
    client2->disconnect();
    client3->disconnect();
}

// 测试客户端订阅独立性
TEST_F(IntrospectionClientTest, MultipleClientsSubscription) {
    auto client1 = std::make_unique<IntrospectionClient>();
    auto client2 = std::make_unique<IntrospectionClient>();

    EXPECT_TRUE(client1->connectLocal(server));
    EXPECT_TRUE(client2->connectLocal(server));

    std::atomic<int> count1{0};
    std::atomic<int> count2{0};

    auto callback1 = [&count1](const IntrospectionEvent&) { count1++; };
    auto callback2 = [&count2](const IntrospectionEvent&) { count2++; };

    EXPECT_TRUE(client1->subscribe(callback1));
    EXPECT_TRUE(client2->subscribe(callback2));

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 两个客户端都应该收到事件
    EXPECT_GE(count1.load(), 2);
    EXPECT_GE(count2.load(), 2);

    // 取消一个客户端的订阅
    client1->unsubscribe();

    int count1_before = count1.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // client1 不应该再收到事件
    EXPECT_EQ(count1.load(), count1_before);
    // client2 继续收到事件
    EXPECT_GT(count2.load(), count1_before);

    client1->disconnect();
    client2->disconnect();
}

// 测试服务器停止时客户端行为
TEST_F(IntrospectionClientTest, ServerStopWhileConnected) {
    EXPECT_TRUE(client->connectLocal(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 停止服务器
    server->stop();

    // 客户端仍然是"连接"状态，但查询应该失败或返回空数据
    EXPECT_TRUE(client->isConnected());

    client->disconnect();
}

// 测试并发查询
TEST_F(IntrospectionClientTest, ConcurrentQueries) {
    EXPECT_TRUE(client->connectLocal(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    std::atomic<bool> stop{false};

    auto query_thread = [this, &success_count, &error_count, &stop]() {
        while (!stop) {
            SystemMetrics metrics;
            if (client->getMetrics(metrics)) {
                if (metrics.memory.total_memory > 0) {
                    success_count++;
                } else {
                    error_count++;
                }
            } else {
                error_count++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    // 启动多个查询线程
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(query_thread);
    }

    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    stop = true;
    for (auto& t : threads) {
        t.join();
    }

    // 应该有成功的查询
    EXPECT_GT(success_count.load(), 0);
    // 不应该有错误
    EXPECT_EQ(error_count.load(), 0);

    client->disconnect();
}

// 测试客户端生命周期
TEST_F(IntrospectionClientTest, ClientLifecycle) {
    {
        auto temp_client = std::make_unique<IntrospectionClient>();
        EXPECT_TRUE(temp_client->connectLocal(server));
        EXPECT_TRUE(temp_client->isConnected());
        // temp_client 析构时应该自动断开连接
    }

    // 创建新客户端应该可以正常连接
    auto new_client = std::make_unique<IntrospectionClient>();
    EXPECT_TRUE(new_client->connectLocal(server));
    EXPECT_TRUE(new_client->isConnected());
    
    new_client->disconnect();
}

// 测试空回调
TEST_F(IntrospectionClientTest, NullCallback) {
    EXPECT_TRUE(client->connectLocal(server));

    // 订阅空回调不应该崩溃
    EventCallback null_callback = nullptr;
    // 根据实现，可能接受或拒绝空回调
    // 这里只确保不崩溃
    client->subscribe(null_callback);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    client->disconnect();
}

