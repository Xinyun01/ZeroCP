/**
 * @file test_introspection_server.cpp
 * @brief 测试 IntrospectionServer 类
 */

#include <gtest/gtest.h>
#include "introspection/introspection_server.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace zero_copy::introspection;

class IntrospectionServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_shared<IntrospectionServer>();
    }

    void TearDown() override {
        if (server->getState() == IntrospectionState::RUNNING) {
            server->stop();
        }
        server.reset();
    }

    std::shared_ptr<IntrospectionServer> server;
};

// 测试服务器启动和停止
TEST_F(IntrospectionServerTest, StartAndStop) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;

    EXPECT_EQ(server->getState(), IntrospectionState::STOPPED);
    
    EXPECT_TRUE(server->start(config));
    EXPECT_EQ(server->getState(), IntrospectionState::RUNNING);

    server->stop();
    EXPECT_EQ(server->getState(), IntrospectionState::STOPPED);
}

// 测试重复启动
TEST_F(IntrospectionServerTest, DoubleStart) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;

    EXPECT_TRUE(server->start(config));
    EXPECT_EQ(server->getState(), IntrospectionState::RUNNING);

    // 第二次启动应该失败
    EXPECT_FALSE(server->start(config));
    EXPECT_EQ(server->getState(), IntrospectionState::RUNNING);

    server->stop();
}

// 测试重复停止
TEST_F(IntrospectionServerTest, DoubleStop) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;

    EXPECT_TRUE(server->start(config));
    server->stop();
    EXPECT_EQ(server->getState(), IntrospectionState::STOPPED);

    // 第二次停止不应该崩溃
    server->stop();
    EXPECT_EQ(server->getState(), IntrospectionState::STOPPED);
}

// 测试获取配置
TEST_F(IntrospectionServerTest, GetConfig) {
    IntrospectionConfig config;
    config.update_interval_ms = 2000;
    config.process_filter = {"test_process"};
    config.connection_filter = {8080, 9090};

    EXPECT_TRUE(server->start(config));

    IntrospectionConfig retrieved = server->getConfig();
    EXPECT_EQ(retrieved.update_interval_ms, 2000);
    EXPECT_EQ(retrieved.process_filter.size(), 1);
    EXPECT_EQ(retrieved.process_filter[0], "test_process");
    EXPECT_EQ(retrieved.connection_filter.size(), 2);

    server->stop();
}

// 测试更新配置
TEST_F(IntrospectionServerTest, UpdateConfig) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;

    EXPECT_TRUE(server->start(config));

    // 更新配置
    IntrospectionConfig new_config;
    new_config.update_interval_ms = 500;
    new_config.process_filter = {"nginx"};

    EXPECT_TRUE(server->updateConfig(new_config));

    // 验证配置已更新
    IntrospectionConfig retrieved = server->getConfig();
    EXPECT_EQ(retrieved.update_interval_ms, 500);
    EXPECT_EQ(retrieved.process_filter.size(), 1);
    EXPECT_EQ(retrieved.process_filter[0], "nginx");

    server->stop();
}

// 测试立即收集数据
TEST_F(IntrospectionServerTest, CollectOnce) {
    IntrospectionConfig config;
    config.update_interval_ms = 5000;  // 长间隔

    EXPECT_TRUE(server->start(config));

    // 立即收集数据
    SystemMetrics metrics = server->collectOnce();

    // 验证收集到的数据
    EXPECT_GT(metrics.memory.total_memory, 0);
    // 进程列表应该不为空（至少有测试进程自己）
    EXPECT_GT(metrics.processes.size(), 0);

    server->stop();
}

// 测试获取当前指标
TEST_F(IntrospectionServerTest, GetCurrentMetrics) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;

    EXPECT_TRUE(server->start(config));

    // 等待第一次数据收集
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    SystemMetrics metrics = server->getCurrentMetrics();

    // 验证数据
    EXPECT_GT(metrics.memory.total_memory, 0);
    EXPECT_GE(metrics.memory.memory_usage_percent, 0.0);
    EXPECT_LE(metrics.memory.memory_usage_percent, 100.0);

    server->stop();
}

// 测试回调注册和注销
TEST_F(IntrospectionServerTest, CallbackRegistration) {
    IntrospectionConfig config;
    config.update_interval_ms = 500;

    EXPECT_TRUE(server->start(config));

    std::atomic<int> callback_count{0};

    // 注册回调
    auto callback = [&callback_count](const IntrospectionEvent& event) {
        if (event.type == IntrospectionEventType::SYSTEM_UPDATE) {
            callback_count++;
        }
    };

    uint32_t callback_id = server->registerCallback(callback);
    EXPECT_GT(callback_id, 0);

    // 等待一些回调
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 应该收到至少 2 次回调（1500ms / 500ms = 3）
    EXPECT_GE(callback_count.load(), 2);

    // 注销回调
    server->unregisterCallback(callback_id);

    int count_before_unregister = callback_count.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 注销后不应该再收到回调
    EXPECT_EQ(callback_count.load(), count_before_unregister);

    server->stop();
}

// 测试多个回调
TEST_F(IntrospectionServerTest, MultipleCallbacks) {
    IntrospectionConfig config;
    config.update_interval_ms = 500;

    EXPECT_TRUE(server->start(config));

    std::atomic<int> callback1_count{0};
    std::atomic<int> callback2_count{0};
    std::atomic<int> callback3_count{0};

    auto callback1 = [&callback1_count](const IntrospectionEvent&) {
        callback1_count++;
    };

    auto callback2 = [&callback2_count](const IntrospectionEvent&) {
        callback2_count++;
    };

    auto callback3 = [&callback3_count](const IntrospectionEvent&) {
        callback3_count++;
    };

    uint32_t id1 = server->registerCallback(callback1);
    uint32_t id2 = server->registerCallback(callback2);
    uint32_t id3 = server->registerCallback(callback3);

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 所有回调都应该被调用
    EXPECT_GE(callback1_count.load(), 2);
    EXPECT_GE(callback2_count.load(), 2);
    EXPECT_GE(callback3_count.load(), 2);

    // 注销一个回调
    server->unregisterCallback(id2);

    int count2_before = callback2_count.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // callback2 不应该再被调用
    EXPECT_EQ(callback2_count.load(), count2_before);
    // 其他回调继续工作
    EXPECT_GT(callback1_count.load(), count2_before);
    EXPECT_GT(callback3_count.load(), count2_before);

    server->stop();
}

// 测试进程过滤
TEST_F(IntrospectionServerTest, ProcessFilter) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;
    config.process_filter = {"systemd", "bash", "this_process_should_not_exist_12345"};

    EXPECT_TRUE(server->start(config));

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    SystemMetrics metrics = server->getCurrentMetrics();

    // 验证只包含过滤的进程
    for (const auto& proc : metrics.processes) {
        bool found = false;
        for (const auto& filter : config.process_filter) {
            if (proc.name.find(filter) != std::string::npos) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Process " << proc.name << " should not be in filtered list";
    }

    server->stop();
}

// 测试连接过滤
TEST_F(IntrospectionServerTest, ConnectionFilter) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;
    config.connection_filter = {22, 80, 443, 99999};  // 包含一些常见端口和不存在的端口

    EXPECT_TRUE(server->start(config));

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    SystemMetrics metrics = server->getCurrentMetrics();

    // 验证连接只包含指定端口
    for (const auto& conn : metrics.connections) {
        bool found = false;
        for (uint16_t port : config.connection_filter) {
            if (conn.local_port == port || conn.remote_port == port) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Connection port " << conn.local_port 
                          << " should not be in filtered list";
    }

    server->stop();
}

// 测试短更新间隔
TEST_F(IntrospectionServerTest, ShortUpdateInterval) {
    IntrospectionConfig config;
    config.update_interval_ms = 100;  // 很短的间隔

    EXPECT_TRUE(server->start(config));

    std::atomic<int> callback_count{0};

    auto callback = [&callback_count](const IntrospectionEvent&) {
        callback_count++;
    };

    server->registerCallback(callback);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 1000ms / 100ms = 10 次，考虑误差应该 >= 8
    EXPECT_GE(callback_count.load(), 8);

    server->stop();
}

// 测试线程安全：并发访问
TEST_F(IntrospectionServerTest, ConcurrentAccess) {
    IntrospectionConfig config;
    config.update_interval_ms = 100;

    EXPECT_TRUE(server->start(config));

    std::atomic<bool> stop_threads{false};
    std::atomic<int> error_count{0};

    auto reader_thread = [this, &stop_threads, &error_count]() {
        while (!stop_threads) {
            try {
                SystemMetrics metrics = server->getCurrentMetrics();
                // 验证数据有效性
                if (metrics.memory.total_memory == 0) {
                    error_count++;
                }
            } catch (...) {
                error_count++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    auto config_updater_thread = [this, &stop_threads, &error_count]() {
        while (!stop_threads) {
            try {
                IntrospectionConfig new_config;
                new_config.update_interval_ms = 100 + (rand() % 100);
                server->updateConfig(new_config);
            } catch (...) {
                error_count++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };

    // 启动多个线程
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(reader_thread);
    }
    threads.emplace_back(config_updater_thread);

    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    stop_threads = true;
    for (auto& t : threads) {
        t.join();
    }

    // 不应该有错误
    EXPECT_EQ(error_count.load(), 0);

    server->stop();
}

// 测试停止前未启动
TEST_F(IntrospectionServerTest, StopBeforeStart) {
    EXPECT_EQ(server->getState(), IntrospectionState::STOPPED);
    
    // 停止未启动的服务器不应该崩溃
    server->stop();
    
    EXPECT_EQ(server->getState(), IntrospectionState::STOPPED);
}

// 测试无效配置更新
TEST_F(IntrospectionServerTest, InvalidConfigUpdate) {
    IntrospectionConfig config;
    config.update_interval_ms = 1000;

    // 在未启动状态更新配置应该失败
    EXPECT_FALSE(server->updateConfig(config));

    EXPECT_TRUE(server->start(config));

    // 有效的配置更新应该成功
    IntrospectionConfig new_config;
    new_config.update_interval_ms = 500;
    EXPECT_TRUE(server->updateConfig(new_config));

    server->stop();
}

