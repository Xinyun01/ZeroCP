/**
 * @file daemon_shm_server.cpp
 * @brief 守护进程示例：使用 SOCK_DGRAM 响应共享内存地址查询
 * 
 * 功能：
 * 1. 作为服务端，绑定 Unix Domain Socket
 * 2. 循环接收客户端请求（数据报模式，无连接）
 * 3. 返回共享内存地址信息
 * 
 * 优点：
 * - 简单：无需 listen/accept
 * - 无状态：每个请求独立处理
 * - 适合查询类场景
 */

#include "unix_domainsocket.hpp"
#include "logging.hpp"
#include <iostream>
#include <string>
#include <csignal>
#include <cstring>

using namespace ZeroCP;
using namespace ZeroCP::Details;

// 全局标志，用于优雅退出
static volatile bool g_running = true;

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\n[SERVER] Received shutdown signal, exiting gracefully..." << std::endl;
        g_running = false;
    }
}

/**
 * @brief 处理客户端请求
 * @param request 请求内容
 * @return 响应内容
 */
std::string processRequest(const std::string& request)
{
    std::cout << "[SERVER] Processing request: " << request << std::endl;
    
    if (request == "GET_SHM_PATH")
    {
        return "/dev/shm/zero_copy_framework_shm";
    }
    else if (request == "GET_SHM_SIZE")
    {
        return "4096";
    }
    else if (request == "PING")
    {
        return "PONG";
    }
    else
    {
        return "ERROR: Unknown command";
    }
}

int main()
{
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Shared Memory Daemon Server (SOCK_DGRAM)                  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    const char* socketPath = "/tmp/shm_daemon.sock";
    
    // 创建服务端套接字（SOCK_DGRAM 模式）
    std::cout << "\n[SERVER] Creating server socket: " << socketPath << std::endl;
    
    auto serverResult = UnixDomainSocketBuilder()
        .name(socketPath)
        .channelSide(PosixIpcChannelSide::SERVER)
        .maxMsgSize(1024)
        .create();
    
    if (!serverResult.has_value())
    {
        std::cerr << "[ERROR] Failed to create server socket" << std::endl;
        return 1;
    }
    
    auto& serverSocket = serverResult.value();
    
    std::cout << "[SERVER] ✅ Server is ready and listening on " << socketPath << std::endl;
    std::cout << "[SERVER] Using SOCK_DGRAM (datagram) mode" << std::endl;
    std::cout << "[SERVER] Press Ctrl+C to stop\n" << std::endl;
    
    // 简单的事件循环
    while (g_running)
    {
        std::cout << "[SERVER] Waiting for request..." << std::endl;
        
        // 接收请求（阻塞）
        std::string request;
        auto recvResult = serverSocket.receive(request);
        
        if (!g_running)
        {
            break;  // 被信号中断
        }
        
        if (!recvResult.has_value())
        {
            std::cerr << "[ERROR] Failed to receive request" << std::endl;
            continue;
        }
        
        if (request.empty())
        {
            std::cout << "[SERVER] Received empty message, skipping..." << std::endl;
            continue;
        }
        
        std::cout << "[SERVER] ✅ Received: \"" << request << "\"" << std::endl;
        
        // 处理请求
        std::string response = processRequest(request);
        
        // 发送响应
        std::cout << "[SERVER] Sending response: \"" << response << "\"" << std::endl;
        auto sendResult = serverSocket.send(response);
        
        if (!sendResult.has_value())
        {
            std::cerr << "[ERROR] Failed to send response" << std::endl;
        }
        else
        {
            std::cout << "[SERVER] ✅ Response sent successfully\n" << std::endl;
        }
    }
    
    std::cout << "\n[SERVER] Shutting down gracefully..." << std::endl;
    
    // 资源会自动清理（RAII）
    
    std::cout << "[SERVER] Server stopped successfully" << std::endl;
    return 0;
}

