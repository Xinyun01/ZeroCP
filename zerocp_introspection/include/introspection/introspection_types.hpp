#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>

namespace zero_copy {
namespace introspection {

/**
 * @brief 内存信息结构
 */
struct MemoryInfo {
    uint64_t total_memory;          // 总内存 (bytes)
    uint64_t used_memory;           // 已使用内存 (bytes)
    uint64_t free_memory;           // 空闲内存 (bytes)
    uint64_t shared_memory;         // 共享内存 (bytes)
    uint64_t buffer_memory;         // 缓冲区内存 (bytes)
    uint64_t cached_memory;         // 缓存内存 (bytes)
    double memory_usage_percent;    // 内存使用率 (%)
};

/**
 * @brief 进程信息结构
 */
struct ProcessInfo {
    uint32_t pid;                   // 进程ID
    std::string name;               // 进程名称
    std::string command_line;       // 命令行
    uint64_t memory_usage;          // 内存使用量 (bytes)
    double cpu_usage;               // CPU使用率 (%)
    std::string state;              // 进程状态
    uint64_t start_time;            // 启动时间
    uint32_t threads_count;         // 线程数量
};

/**
 * @brief 连接信息结构
 */
struct ConnectionInfo {
    std::string local_address;      // 本地地址
    std::string remote_address;     // 远程地址
    std::string protocol;           // 协议 (TCP/UDP)
    std::string state;              // 连接状态
    uint64_t bytes_sent;            // 发送字节数
    uint64_t bytes_received;        // 接收字节数
    uint32_t pid;                   // 关联进程ID
};

/**
 * @brief 系统负载信息结构
 */
struct LoadInfo {
    double load_1min;               // 1分钟平均负载
    double load_5min;               // 5分钟平均负载
    double load_15min;              // 15分钟平均负载
    uint32_t cpu_count;             // CPU核心数
    double cpu_usage_percent;       // 整体CPU使用率 (%)
};

/**
 * @brief 系统监控数据结构（聚合所有信息）
 */
struct SystemMetrics {
    MemoryInfo memory;                          // 内存信息
    std::vector<ProcessInfo> processes;         // 进程列表
    std::vector<ConnectionInfo> connections;    // 连接列表
    LoadInfo load;                              // 系统负载
    std::chrono::system_clock::time_point timestamp; // 时间戳
};

/**
 * @brief 监控配置选项
 */
struct IntrospectionConfig {
    uint32_t update_interval_ms = 1000;         // 更新间隔 (毫秒)
    std::vector<std::string> process_filter;    // 进程名称过滤器
    std::vector<uint16_t> connection_filter;    // 连接端口过滤器
    bool enable_memory_monitoring = true;       // 启用内存监控
    bool enable_process_monitoring = true;      // 启用进程监控
    bool enable_connection_monitoring = true;   // 启用连接监控
    bool enable_load_monitoring = true;         // 启用负载监控
};

/**
 * @brief 监控事件类型
 */
enum class IntrospectionEventType {
    MEMORY_UPDATE,          // 内存信息更新
    PROCESS_UPDATE,         // 进程信息更新
    CONNECTION_UPDATE,      // 连接信息更新
    LOAD_UPDATE,            // 负载信息更新
    SYSTEM_UPDATE,          // 完整系统更新
    ERROR                   // 错误事件
};

/**
 * @brief 监控事件结构
 */
struct IntrospectionEvent {
    IntrospectionEventType type;                // 事件类型
    SystemMetrics metrics;                      // 系统指标数据
    std::string error_message;                  // 错误信息（仅当type为ERROR时使用）
    std::chrono::system_clock::time_point timestamp; // 事件时间戳
};

/**
 * @brief 监控状态
 */
enum class IntrospectionState {
    STOPPED,        // 已停止
    STARTING,       // 正在启动
    RUNNING,        // 运行中
    STOPPING,       // 正在停止
    ERROR           // 错误状态
};

} // namespace introspection
} // namespace zero_copy

