#pragma once

#include "introspection_types.hpp"
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

namespace zero_copy {
namespace introspection {

/**
 * @brief Introspection 服务端接口
 * 
 * 负责收集系统运行时监控数据并提供给客户端
 * 采用发布-订阅模式，支持多个客户端同时订阅
 */
class IntrospectionServer {
public:
    /**
     * @brief 事件回调函数类型
     */
    using EventCallback = std::function<void(const IntrospectionEvent&)>;

    IntrospectionServer();
    ~IntrospectionServer();

    // 禁止拷贝和移动
    IntrospectionServer(const IntrospectionServer&) = delete;
    IntrospectionServer& operator=(const IntrospectionServer&) = delete;

    /**
     * @brief 启动监控服务
     * @param config 监控配置
     * @return true 成功启动
     */
    bool start(const IntrospectionConfig& config);

    /**
     * @brief 停止监控服务
     */
    void stop();

    /**
     * @brief 获取当前状态
     * @return 当前服务状态
     */
    IntrospectionState getState() const;

    /**
     * @brief 获取当前系统指标（同步接口）
     * @return 系统指标数据
     */
    SystemMetrics getCurrentMetrics() const;

    /**
     * @brief 注册事件回调（异步接口）
     * @param callback 回调函数
     * @return 回调ID，用于取消注册
     */
    uint32_t registerCallback(EventCallback callback);

    /**
     * @brief 取消注册事件回调
     * @param callback_id 回调ID
     */
    void unregisterCallback(uint32_t callback_id);

    /**
     * @brief 更新配置
     * @param config 新的监控配置
     * @return true 更新成功
     */
    bool updateConfig(const IntrospectionConfig& config);

    /**
     * @brief 获取当前配置
     * @return 当前监控配置
     */
    IntrospectionConfig getConfig() const;

    /**
     * @brief 手动触发一次数据收集
     * @return 收集到的系统指标
     */
    SystemMetrics collectOnce();

private:
    /**
     * @brief 监控线程主循环
     */
    void monitoringLoop();

    /**
     * @brief 收集系统指标
     */
    SystemMetrics collectMetrics();

    /**
     * @brief 收集内存信息
     */
    void collectMemoryInfo(MemoryInfo& mem_info);

    /**
     * @brief 收集进程信息
     */
    void collectProcessInfo(std::vector<ProcessInfo>& processes);

    /**
     * @brief 收集连接信息
     */
    void collectConnectionInfo(std::vector<ConnectionInfo>& connections);

    /**
     * @brief 收集系统负载信息
     */
    void collectLoadInfo(LoadInfo& load_info);

    /**
     * @brief 通知所有回调
     */
    void notifyCallbacks(const IntrospectionEvent& event);

    /**
     * @brief 读取 /proc/meminfo
     */
    bool readMemInfo(MemoryInfo& mem_info);

    /**
     * @brief 读取 /proc/loadavg
     */
    bool readLoadAvg(LoadInfo& load_info);

    /**
     * @brief 读取进程信息
     */
    bool readProcessInfo(uint32_t pid, ProcessInfo& proc_info);

    /**
     * @brief 读取网络连接信息
     */
    bool readNetworkConnections(std::vector<ConnectionInfo>& connections);

    /**
     * @brief 应用进程过滤器
     */
    void applyProcessFilter(std::vector<ProcessInfo>& processes);

    /**
     * @brief 应用连接过滤器
     */
    void applyConnectionFilter(std::vector<ConnectionInfo>& connections);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace introspection
} // namespace zero_copy

