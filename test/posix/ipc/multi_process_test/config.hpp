/**
 * @file config.hpp
 * @brief 多进程 Unix Domain Socket 通信测试的共享配置
 * 
 * 所有客户端和服务端通过此配置文件获取服务端 socket 路径等配置信息
 */

#ifndef MULTI_PROCESS_TEST_CONFIG_HPP
#define MULTI_PROCESS_TEST_CONFIG_HPP

namespace TestConfig
{
    // ============================================================================
    // 服务端配置
    // ============================================================================
    
    /// @brief 服务端 Unix Domain Socket 路径（所有客户端需要连接的地址）
    constexpr const char* SERVER_SOCKET_PATH = "/tmp/uds_multi_process_server.sock";
    
    /// @brief 服务端名称（用于日志）
    constexpr const char* SERVER_NAME = "MultiProcessServer";
    
    // ============================================================================
    // 客户端配置
    // ============================================================================
    
    /// @brief 客户端 socket 路径前缀
    /// 每个客户端会创建形如 "/tmp/uds_client_1.sock" 的本地 socket
    constexpr const char* CLIENT_SOCKET_PREFIX = "/tmp/uds_client_";
    
    /// @brief 客户端名称前缀（用于日志）
    constexpr const char* CLIENT_NAME_PREFIX = "Client-";
    
    // ============================================================================
    // 通信配置
    // ============================================================================
    
    /// @brief 最大消息大小（字节）
    constexpr uint64_t MAX_MESSAGE_SIZE = 256;
    
    /// @brief 每个客户端发送的消息数量
    constexpr int MESSAGES_PER_CLIENT = 5;
    
    /// @brief 客户端启动延迟（毫秒）- 确保服务端先启动
    constexpr int CLIENT_STARTUP_DELAY_MS = 500;
    
    /// @brief 消息发送间隔（毫秒）- 模拟真实场景
    constexpr int MESSAGE_INTERVAL_MS = 100;
    
    // ============================================================================
    // 测试配置
    // ============================================================================
    
    /// @brief 客户端数量
    constexpr int NUM_CLIENTS = 5;
    
    /// @brief 服务端超时时间（秒）
    constexpr int SERVER_TIMEOUT_SECONDS = 30;
    
    /// @brief 是否启用详细日志
    constexpr bool VERBOSE_LOGGING = true;

} // namespace TestConfig

#endif // MULTI_PROCESS_TEST_CONFIG_HPP

