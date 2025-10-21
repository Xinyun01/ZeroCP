/**
 * @file client5.cpp
 * @brief 客户端程序 #5
 * 
 * 功能：
 * - 创建 SOCK_DGRAM 类型的客户端 socket
 * - 绑定本地地址（用于接收服务端响应）
 * - 使用 sendTo() 发送消息到服务端
 * - 使用 receiveFrom() 接收服务端响应
 * - 发送多条消息并验证响应
 * 
 * 编译：
 *   make client5
 * 
 * 运行：
 *   ./client5
 */

#include "unix_domainsocket.hpp"
#include "logging.hpp"
#include "config.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

using namespace ZeroCP;
using namespace ZeroCP::Details;
using namespace TestConfig;

// 客户端 ID
constexpr int CLIENT_ID = 5;

/**
 * @brief 主函数
 */
int main()
{
    // 等待服务端启动
    std::this_thread::sleep_for(std::chrono::milliseconds(CLIENT_STARTUP_DELAY_MS));

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                   Client #" << CLIENT_ID << " Starting                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Client Configuration:\n";
    std::cout << "  - Client ID:         " << CLIENT_ID << "\n";
    std::cout << "  - Server Path:       " << SERVER_SOCKET_PATH << "\n";
    std::cout << "  - Process ID:        " << getpid() << "\n";
    std::cout << "  - Messages to Send:  " << MESSAGES_PER_CLIENT << "\n";
    std::cout << "\n";

    // 创建客户端本地 socket 路径
    std::string clientSocketPath = std::string(CLIENT_SOCKET_PREFIX) + std::to_string(CLIENT_ID) + ".sock";
    
    // 清理旧的 socket 文件
    unlink(clientSocketPath.c_str());

    // 创建客户端 socket（CLIENT 模式并绑定本地地址）
    UnixDomainSocket::UdsName_t clientName;
    clientName.insert(0, clientSocketPath.c_str());
    
    auto clientResult = UnixDomainSocketBuilder()
        .name(clientName)
        .channelSide(PosixIpcChannelSide::CLIENT)  // CLIENT 模式
        .maxMsgSize(MAX_MESSAGE_SIZE)
        .create();

    if (!clientResult.has_value())
    {
        std::cerr << "[CLIENT-" << CLIENT_ID << "] ❌ Failed to create client socket\n";
        std::cerr << "[CLIENT-" << CLIENT_ID << "] Error code: " << static_cast<int>(clientResult.error()) << "\n";
        return 1;
    }

    auto client = std::move(clientResult.value());
    std::cout << "[CLIENT-" << CLIENT_ID << "] ✅ Client socket created and bound to: " << clientSocketPath << "\n";
    std::cout << "[CLIENT-" << CLIENT_ID << "] 🔗 Ready to communicate with server\n\n";

    // 准备服务端地址
    sockaddr_un serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    strncpy(serverAddr.sun_path, SERVER_SOCKET_PATH, sizeof(serverAddr.sun_path) - 1);

    int successfulExchanges = 0;
    int failedExchanges = 0;

    // 发送多条消息并接收响应
    for (int i = 1; i <= MESSAGES_PER_CLIENT; ++i)
    {
        // 构造消息
        std::string message = "Client-" + std::to_string(CLIENT_ID) + " Message-" + std::to_string(i);

        // 发送消息到服务端
        std::cout << "[CLIENT-" << CLIENT_ID << "] 📤 [" << i << "/" << MESSAGES_PER_CLIENT 
                  << "] Sending: \"" << message << "\"\n";
        
        auto sendResult = client.sendTo(message, serverAddr);
        
        if (!sendResult.has_value())
        {
            std::cerr << "[CLIENT-" << CLIENT_ID << "] ❌ Failed to send message\n";
            std::cerr << "[CLIENT-" << CLIENT_ID << "] Error code: " << static_cast<int>(sendResult.error()) << "\n";
            failedExchanges++;
            continue;
        }

        std::cout << "[CLIENT-" << CLIENT_ID << "] ✅ Message sent successfully\n";

        // 等待服务端响应
        std::string response;
        sockaddr_un fromAddr;
        
        std::cout << "[CLIENT-" << CLIENT_ID << "] ⏳ Waiting for server response...\n";
        
        auto recvResult = client.receiveFrom(response, fromAddr);
        
        if (!recvResult.has_value())
        {
            std::cerr << "[CLIENT-" << CLIENT_ID << "] ❌ Failed to receive response\n";
            std::cerr << "[CLIENT-" << CLIENT_ID << "] Error code: " << static_cast<int>(recvResult.error()) << "\n";
            failedExchanges++;
            continue;
        }

        std::cout << "[CLIENT-" << CLIENT_ID << "] 📨 [" << i << "/" << MESSAGES_PER_CLIENT 
                  << "] Received: \"" << response << "\" ✅\n";
        
        // 验证响应来自服务端
        std::string responsePath = fromAddr.sun_path;
        if (responsePath == SERVER_SOCKET_PATH)
        {
            std::cout << "[CLIENT-" << CLIENT_ID << "] ✅ Response verified from server: " 
                      << responsePath << "\n\n";
            successfulExchanges++;
        }
        else
        {
            std::cerr << "[CLIENT-" << CLIENT_ID << "] ⚠️  Unexpected response source: " 
                      << responsePath << "\n\n";
        }

        // 消息发送间隔
        if (i < MESSAGES_PER_CLIENT)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_INTERVAL_MS));
        }
    }

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "[CLIENT-" << CLIENT_ID << "] 📊 Client Statistics\n";
    std::cout << "========================================\n";
    std::cout << "Successful Exchanges: " << successfulExchanges << " / " << MESSAGES_PER_CLIENT << "\n";
    std::cout << "Failed Exchanges:     " << failedExchanges << " / " << MESSAGES_PER_CLIENT << "\n";
    std::cout << "Success Rate:         " << (successfulExchanges * 100 / MESSAGES_PER_CLIENT) << "%\n";
    std::cout << "========================================\n";
    std::cout << "[CLIENT-" << CLIENT_ID << "] " 
              << (successfulExchanges == MESSAGES_PER_CLIENT ? "✅ All exchanges completed successfully!" : "⚠️  Some exchanges failed!") 
              << "\n";
    std::cout << "========================================\n\n";

    // 清理 socket 文件
    unlink(clientSocketPath.c_str());

    return (successfulExchanges == MESSAGES_PER_CLIENT) ? 0 : 1;
}

