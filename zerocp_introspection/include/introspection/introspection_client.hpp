#pragma once

#include "introspection_types.hpp"
#include <memory>
#include <functional>

namespace zero_copy {
namespace introspection {

// 前向声明
class IntrospectionServer;

/**
 * @brief Introspection 客户端接口
 * 
 * 提供访问监控数据的客户端接口
 * 支持同步查询和异步订阅两种模式
 */
class IntrospectionClient {
public:
    /**
     * @brief 事件回调函数类型
     */
    using EventCallback = std::function<void(const IntrospectionEvent&)>;

    IntrospectionClient();
    ~IntrospectionClient();

    // 禁止拷贝和移动
    IntrospectionClient(const IntrospectionClient&) = delete;
    IntrospectionClient& operator=(const IntrospectionClient&) = delete;

    /**
     * @brief 连接到本地服务端
     * @param server 服务端实例指针
     * @return true 连接成功
     */
    bool connectLocal(std::shared_ptr<IntrospectionServer> server);

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 检查是否已连接
     * @return true 已连接
     */
    bool isConnected() const;

    /**
     * @brief 获取当前系统指标（同步查询）
     * @param metrics 输出参数，存储获取到的指标
     * @return true 获取成功
     */
    bool getMetrics(SystemMetrics& metrics);

    /**
     * @brief 获取内存信息
     * @param memory 输出参数，存储获取到的内存信息
     * @return true 获取成功
     */
    bool getMemoryInfo(MemoryInfo& memory);

    /**
     * @brief 获取进程列表
     * @param processes 输出参数，存储获取到的进程列表
     * @return true 获取成功
     */
    bool getProcessList(std::vector<ProcessInfo>& processes);

    /**
     * @brief 获取连接列表
     * @param connections 输出参数，存储获取到的连接列表
     * @return true 获取成功
     */
    bool getConnectionList(std::vector<ConnectionInfo>& connections);

    /**
     * @brief 获取系统负载信息
     * @param load 输出参数，存储获取到的负载信息
     * @return true 获取成功
     */
    bool getLoadInfo(LoadInfo& load);

    /**
     * @brief 订阅监控事件（异步接口）
     * @param callback 事件回调函数
     * @return true 订阅成功
     */
    bool subscribe(EventCallback callback);

    /**
     * @brief 取消订阅
     */
    void unsubscribe();

    /**
     * @brief 请求更新配置
     * @param config 新的监控配置
     * @return true 更新成功
     */
    bool requestConfigUpdate(const IntrospectionConfig& config);

    /**
     * @brief 获取当前配置
     * @param config 输出参数，存储获取到的配置
     * @return true 获取成功
     */
    bool getConfig(IntrospectionConfig& config);

    /**
     * @brief 请求立即收集一次数据
     * @param metrics 输出参数，存储收集到的指标
     * @return true 收集成功
     */
    bool requestCollectOnce(SystemMetrics& metrics);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace introspection
} // namespace zero_copy

