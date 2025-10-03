/**
 * @file test_integration.cpp
 * @brief 集成测试 - 测试客户端和服务端的完整交互
 */

#include <gtest/gtest.h>
#include "introspection/introspection_client.hpp"
#include "introspection/introspection_server.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <memory>

using namespace zero_copy::introspection;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_shared<IntrospectionServer>();
    }

    void TearDown() override {
        // 清理所有客户端
        for (auto& client : clients) {
            if (client && client->isConnected()) {
                client->disconnect();
            }
        }
        clients.clear();

        // 停止服务器
        if (server && server->getState() == IntrospectionState::RUNNING) {
            server->stop();
        }
        server.reset();
    }

    std::shared_ptr<IntrospectionServer> server;
    std::vector<std::unique_ptr<IntrospectionClient>> clients;
};

// 测试完整的工作流程
TEST_F(IntegrationTest, CompleteWorkflow) {
    // 1. 启动服务器
    IntrospectionConfig config;
    config.update_interval_ms = 500;
    EXPECT_TRUE(server->start(config));

    // 2. 连接客户端
    auto client = std::make_unique<IntrospectionClient>();
    EXPECT_TRUE(client->connectLocal(server));

    // 3. 等待数据收集
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 4. 查询各种数据
    SystemMetrics metrics;
    EXPECT_TRUE(client->getMetrics(metrics));
    EXPECT_GT(metrics.memory.total_memory, 0);

    MemoryInfo memory;
    EXPECT_TRUE(client->getMemoryInfo(memory));
    EXPECT_GT(memory.total_memory, 0);

    std::vector<ProcessInfo> processes;
    EXPECT_TRUE(client->getProcessList(processes));
    EXPECT_GT(processes.size(), 0);

    LoadInfo load;
    EXPECT_TRUE(client->getLoadInfo(load));
    EXPECT_GE(load.cpu_usage_percent, 0.0);

    // 5. 订阅事件
    std::atomic<int> event_count{0};
    client->subscribe([&event_count](const IntrospectionEvent&) {
        event_count++;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    EXPECT_GE(event_count.load(), 2);

    // 6. 更新配置
    IntrospectionConfig new_config;
    new_config.update_interval_ms = 1000;
    EXPECT_TRUE(client->requestConfigUpdate(new_config));

    // 7. 立即收集数据
    SystemMetrics fresh_metrics;
    EXPECT_TRUE(client->requestCollectOnce(fresh_metrics));
    EXPECT_GT(fresh_metrics.memory.total_memory, 0);

    // 8. 清理
    client->unsubscribe();
    client->disconnect();
    server->stop();
}

// 测试多客户端场景
TEST_F(IntegrationTest, MultipleClientsScenario) {
    // 启动服务器
    IntrospectionConfig config;
    config.update_interval_ms = 500;
    EXPECT_TRUE(server->start(config));

    // 创建多个客户端
    const int num_clients = 5;
    for (int i = 0; i < num_clients; ++i) {
        auto client = std::make_unique<IntrospectionClient>();
        EXPECT_TRUE(client->connectLocal(server));
        clients.push_back(std::move(client));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 所有客户端并发查询
    std::vector<SystemMetrics> all_metrics(num_clients);
    std::vector<std::thread> query_threads;

    for (int i = 0; i < num_clients; ++i) {
        query_threads.emplace_back([this, i, &all_metrics]() {
            EXPECT_TRUE(clients[i]->getMetrics(all_metrics[i]));
        });
    }

    for (auto& t : query_threads) {
        t.join();
    }

    // 验证所有客户端获取的数据一致
    for (int i = 1; i < num_clients; ++i) {
        EXPECT_EQ(all_metrics[0].memory.total_memory, 
                  all_metrics[i].memory.total_memory);
    }

    // 清理
    for (auto& client : clients) {
        client->disconnect();
    }
    server->stop();
}

// 测试事件广播
TEST_F(IntegrationTest, EventBroadcast) {
    IntrospectionConfig config;
    config.update_interval_ms = 500;
    EXPECT_TRUE(server->start(config));

    // 创建多个客户端并订阅
    const int num_clients = 3;
    std::vector<std::atomic<int>> event_counts(num_clients);

    for (int i = 0; i < num_clients; ++i) {
        auto client = std::make_unique<IntrospectionClient>();
        EXPECT_TRUE(client->connectLocal(server));

        int client_index = i;
        client->subscribe([&event_counts, client_index](const IntrospectionEvent&) {
            event_counts[client_index]++;
        });

        clients.push_back(std::move(client));
    }

    // 等待事件
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 所有客户端都应该收到事件
    for (int i = 0; i < num_clients; ++i) {
        EXPECT_GE(event_counts[i].load(), 2) 
            << "Client " << i << " did not receive enough events";
    }

    // 清理
    for (auto& client : clients) {
        client->unsubscribe();
        client->disconnect();
    }
    server->stop();
}

// 测试配置动态更新
TEST_F(IntegrationTest, DynamicConfigUpdate) {
    IntrospectionConfig config;
    config.update_interval_ms = 500;
    EXPECT_TRUE(server->start(config));

    auto client = std::make_unique<IntrospectionClient>();
    EXPECT_TRUE(client->connectLocal(server));

    std::atomic<int> event_count{0};
    auto last_update_time = std::chrono::steady_clock::now();
    std::atomic<int64_t> avg_interval_ms{0};

    client->subscribe([&](const IntrospectionEvent&) {
        auto now = std::chrono::steady_clock::now();
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_update_time).count();
        last_update_time = now;
        
        if (event_count > 0) {  // 忽略第一次
            avg_interval_ms = interval;
        }
        event_count++;
    });

    // 等待几次更新
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 修改更新间隔
    IntrospectionConfig new_config;
    new_config.update_interval_ms = 1000;  // 从 500ms 改为 1000ms
    EXPECT_TRUE(client->requestConfigUpdate(new_config));

    event_count = 0;
    last_update_time = std::chrono::steady_clock::now();

    // 等待新间隔下的更新
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    // 验证新的更新间隔（允许一定误差）
    EXPECT_GE(avg_interval_ms.load(), 800);  // 至少 800ms
    EXPECT_LE(avg_interval_ms.load(), 1200);  // 最多 1200ms

    client->disconnect();
    server->stop();
}

// 测试进程过滤功能
TEST_F(IntegrationTest, ProcessFiltering) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;
    config.process_filter = {"systemd", "bash"};
    EXPECT_TRUE(server->start(config));

    auto client = std::make_unique<IntrospectionClient>();
    EXPECT_TRUE(client->connectLocal(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    std::vector<ProcessInfo> processes;
    EXPECT_TRUE(client->getProcessList(processes));

    // 验证所有进程都符合过滤条件
    for (const auto& proc : processes) {
        bool matches = false;
        for (const auto& filter : config.process_filter) {
            if (proc.name.find(filter) != std::string::npos) {
                matches = true;
                break;
            }
        }
        EXPECT_TRUE(matches) << "Process " << proc.name 
                            << " does not match any filter";
    }

    client->disconnect();
    server->stop();
}

// 测试长时间运行稳定性
TEST_F(IntegrationTest, LongRunningStability) {
    IntrospectionConfig config;
    config.update_interval_ms = 100;  // 快速更新
    EXPECT_TRUE(server->start(config));

    auto client = std::make_unique<IntrospectionClient>();
    EXPECT_TRUE(client->connectLocal(server));

    std::atomic<int> event_count{0};
    std::atomic<int> error_count{0};

    client->subscribe([&](const IntrospectionEvent& event) {
        event_count++;
        // 验证数据有效性
        if (event.metrics.memory.total_memory == 0) {
            error_count++;
        }
    });

    // 运行 5 秒
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 应该收到大量事件（5000ms / 100ms = 50，考虑误差至少 40）
    EXPECT_GE(event_count.load(), 40);
    
    // 不应该有错误
    EXPECT_EQ(error_count.load(), 0);

    client->disconnect();
    server->stop();
}

// 测试客户端重连
TEST_F(IntegrationTest, ClientReconnection) {
    IntrospectionConfig config;
    config.update_interval_ms = 500;
    EXPECT_TRUE(server->start(config));

    auto client = std::make_unique<IntrospectionClient>();

    // 第一次连接
    EXPECT_TRUE(client->connectLocal(server));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    SystemMetrics metrics1;
    EXPECT_TRUE(client->getMetrics(metrics1));

    // 断开
    client->disconnect();
    EXPECT_FALSE(client->isConnected());

    // 重新连接
    EXPECT_TRUE(client->connectLocal(server));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    SystemMetrics metrics2;
    EXPECT_TRUE(client->getMetrics(metrics2));

    // 两次获取的数据应该有效
    EXPECT_GT(metrics1.memory.total_memory, 0);
    EXPECT_GT(metrics2.memory.total_memory, 0);

    client->disconnect();
    server->stop();
}

// 测试服务器重启
TEST_F(IntegrationTest, ServerRestart) {
    IntrospectionConfig config;
    config.update_interval_ms = 500;

    // 第一次启动
    EXPECT_TRUE(server->start(config));

    auto client = std::make_unique<IntrospectionClient>();
    EXPECT_TRUE(client->connectLocal(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    SystemMetrics metrics1;
    EXPECT_TRUE(client->getMetrics(metrics1));

    // 停止服务器
    server->stop();
    EXPECT_EQ(server->getState(), IntrospectionState::STOPPED);

    // 客户端断开
    client->disconnect();

    // 重新启动服务器
    EXPECT_TRUE(server->start(config));
    EXPECT_EQ(server->getState(), IntrospectionState::RUNNING);

    // 重新连接客户端
    EXPECT_TRUE(client->connectLocal(server));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    SystemMetrics metrics2;
    EXPECT_TRUE(client->getMetrics(metrics2));

    EXPECT_GT(metrics2.memory.total_memory, 0);

    client->disconnect();
    server->stop();
}

// 测试并发配置更新
TEST_F(IntegrationTest, ConcurrentConfigUpdates) {
    IntrospectionConfig config;
    config.update_interval_ms = 500;
    EXPECT_TRUE(server->start(config));

    // 创建多个客户端
    const int num_clients = 3;
    for (int i = 0; i < num_clients; ++i) {
        auto client = std::make_unique<IntrospectionClient>();
        EXPECT_TRUE(client->connectLocal(server));
        clients.push_back(std::move(client));
    }

    std::atomic<int> update_count{0};
    std::atomic<bool> stop{false};

    // 多个线程并发更新配置
    auto updater = [this, &update_count, &stop]() {
        while (!stop) {
            IntrospectionConfig new_config;
            new_config.update_interval_ms = 400 + (rand() % 200);  // 400-600ms
            
            for (auto& client : clients) {
                if (client->requestConfigUpdate(new_config)) {
                    update_count++;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };

    std::vector<std::thread> updater_threads;
    for (int i = 0; i < 3; ++i) {
        updater_threads.emplace_back(updater);
    }

    // 运行 2 秒
    std::this_thread::sleep_for(std::chrono::seconds(2));

    stop = true;
    for (auto& t : updater_threads) {
        t.join();
    }

    // 应该有成功的更新
    EXPECT_GT(update_count.load(), 0);

    // 验证服务器仍在正常运行
    EXPECT_EQ(server->getState(), IntrospectionState::RUNNING);

    for (auto& client : clients) {
        client->disconnect();
    }
    server->stop();
}

// 测试内存泄漏检测（简单版）
TEST_F(IntegrationTest, NoMemoryLeak) {
    IntrospectionConfig config;
    config.update_interval_ms = 100;

    // 多次启动-停止循环
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(server->start(config));

        auto client = std::make_unique<IntrospectionClient>();
        EXPECT_TRUE(client->connectLocal(server));

        client->subscribe([](const IntrospectionEvent&) {});

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        client->unsubscribe();
        client->disconnect();
        server->stop();
    }

    // 如果有内存泄漏，多次循环后可能会出现问题
    // 这个测试主要确保不会崩溃
    EXPECT_EQ(server->getState(), IntrospectionState::STOPPED);
}

// 测试异常处理
TEST_F(IntegrationTest, ExceptionHandling) {
    IntrospectionConfig config;
    config.update_interval_ms = 500;
    EXPECT_TRUE(server->start(config));

    auto client = std::make_unique<IntrospectionClient>();
    EXPECT_TRUE(client->connectLocal(server));

    // 订阅一个会抛出异常的回调
    client->subscribe([](const IntrospectionEvent&) {
        // 抛出异常不应该影响其他客户端
        throw std::runtime_error("Test exception");
    });

    // 创建另一个正常的客户端
    auto client2 = std::make_unique<IntrospectionClient>();
    EXPECT_TRUE(client2->connectLocal(server));

    std::atomic<int> normal_event_count{0};
    client2->subscribe([&normal_event_count](const IntrospectionEvent&) {
        normal_event_count++;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 正常客户端应该仍然收到事件
    EXPECT_GE(normal_event_count.load(), 2);

    client->disconnect();
    client2->disconnect();
    server->stop();
}

