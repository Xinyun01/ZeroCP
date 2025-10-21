/**
 * @file server.cpp
 * @brief 多客户端服务端程序 - 使用 receiveFrom/sendTo API
 * 
 * 功能：
 * - 创建 SOCK_DGRAM 类型的服务端 socket
 * - 持续监听来自多个客户端的消息
 * - 使用 receiveFrom() 接收消息并获取发送者地址
 * - 使用 sendTo() 将响应发送回正确的客户端
 * - 支持优雅退出（SIGINT/SIGTERM）
 * 
 * 编译：
 *   make server
 * 
 * 运行：
 *   ./server
 */

#include "unix_domainsocket.hpp"
#include "logging.hpp"
#include "config.hpp"
#include <iostream>
#include <string>
#include <atomic>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

using namespace ZeroCP;
using namespace ZeroCP::Details;
using namespace TestConfig;

// 全局标志控制服务端运行状态
std::atomic<bool> g_serverRunning{true};

/**
 * @brief 信号处理函数 - 优雅退出
 */
void signalHandler(int signum)
{
    std::cout << "\n[SERVER] 📡 Received signal " << signum << ", shutting down gracefully...\n";
    g_serverRunning = false;
}

/**
 * @brief 主函数
 */
int main()
{
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        Multi-Client Unix Domain Socket Server             ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Server Configuration:\n";
    std::cout << "  - Socket Path:       " << SERVER_SOCKET_PATH << "\n";
    std::cout << "  - Socket Type:       SOCK_DGRAM (无连接)\n";
    std::cout << "  - Max Message Size:  " << MAX_MESSAGE_SIZE << " bytes\n";
    std::cout << "  - Process ID:        " << getpid() << "\n";
    std::cout << "\n";

    // 注册信号处理函数
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 清理旧的 socket 文件
    unlink(SERVER_SOCKET_PATH);

    // 创建服务端 socket
    UnixDomainSocket::UdsName_t socketName;
    socketName.insert(0, SERVER_SOCKET_PATH);
    
    auto serverResult = UnixDomainSocketBuilder()
        .name(socketName)
        .channelSide(PosixIpcChannelSide::SERVER)
        .maxMsgSize(MAX_MESSAGE_SIZE)
        .create();

    if (!serverResult.has_value())
    {
        std::cerr << "[SERVER] ❌ Failed to create server socket\n";
        std::cerr << "[SERVER] Error code: " << static_cast<int>(serverResult.error()) << "\n";
        return 1;
    }

    auto server = std::move(serverResult.value());
    std::cout << "[SERVER] ✅ Server socket created successfully\n";
    std::cout << "[SERVER] 🎧 Listening on: " << SERVER_SOCKET_PATH << "\n";
    std::cout << "[SERVER] ⏳ Waiting for client messages...\n\n";

    int messageCount = 0;
    int expectedMessages = NUM_CLIENTS * MESSAGES_PER_CLIENT;

    // 主循环：接收消息并回复
    while (g_serverRunning)
    {
        std::string receivedMsg;
        sockaddr_un clientAddr;  // 用于存储发送者地址
        
        // 使用 receiveFrom() 接收消息并获取发送者地址
        auto recvResult = server.receiveFrom(receivedMsg, clientAddr);

        if (!recvResult.has_value())
        {
            // 如果收到终止信号，正常退出
            if (!g_serverRunning)
            {
                break;
            }
            
            std::cerr << "[SERVER] ❌ Failed to receive message (error: " 
                      << static_cast<int>(recvResult.error()) << ")\n";
            continue;
        }

        messageCount++;
        
        // 显示接收到的消息和发送者信息
        std::string clientPath = clientAddr.sun_path;
        std::cout << "[SERVER] 📨 [Message " << messageCount << "] Received from: " 
                  << clientPath << "\n";
        std::cout << "[SERVER]    Content: \"" << receivedMsg << "\"\n";

        // 构造响应消息
        std::string response = "ACK: " + receivedMsg;
        
        // 使用 sendTo() 将响应发送回客户端
        auto sendResult = server.sendTo(response, clientAddr);
        
        if (sendResult.has_value())
        {
            std::cout << "[SERVER] 📤 [Message " << messageCount << "] Replied to: " 
                      << clientPath << "\n";
            std::cout << "[SERVER]    Response: \"" << response << "\" ✅\n\n";
        }
        else
        {
            std::cerr << "[SERVER] ❌ Failed to send response to " << clientPath << "\n";
            std::cerr << "[SERVER] Error code: " << static_cast<int>(sendResult.error()) << "\n\n";
        }

        // 如果达到预期消息数量，准备退出
        if (messageCount >= expectedMessages)
        {
            std::cout << "[SERVER] 🎯 Received all expected messages (" << expectedMessages << ")\n";
            std::cout << "[SERVER] Continuing to listen for additional messages...\n";
            std::cout << "[SERVER] Press Ctrl+C to exit.\n\n";
        }
    }

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "[SERVER] 📊 Server Statistics\n";
    std::cout << "========================================\n";
    std::cout << "Total Messages Processed: " << messageCount << "\n";
    std::cout << "Expected Messages:        " << expectedMessages << "\n";
    std::cout << "Success Rate:             " << (messageCount >= expectedMessages ? "✅ 100%" : "⚠️  Partial") << "\n";
    std::cout << "========================================\n";
    std::cout << "[SERVER] 👋 Server shutting down...\n";
    std::cout << "========================================\n\n";

    // 清理 socket 文件
    unlink(SERVER_SOCKET_PATH);

    return (messageCount >= expectedMessages) ? 0 : 1;
}

