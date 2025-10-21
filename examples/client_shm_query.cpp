/**
 * @file client_shm_query.cpp
 * @brief 客户端示例：使用 SOCK_DGRAM 向守护进程查询共享内存地址
 * 
 * 功能：
 * 1. 作为客户端，连接到守护进程的 Unix Domain Socket
 * 2. 发送查询请求
 * 3. 接收共享内存地址信息
 * 4. 展示如何使用获取到的地址
 * 
 * 优点：
 * - 简单：直接 send/receive，无需 connect
 * - 快速：单次请求-响应
 * - 独立：每个客户端独立查询
 */

#include "unix_domainsocket.hpp"
#include "logging.hpp"
#include <iostream>
#include <string>

using namespace ZeroCP;
using namespace ZeroCP::Details;

/**
 * @brief 向守护进程发送请求并获取响应
 * @param socket 套接字
 * @param request 请求内容
 * @return 响应内容，失败返回空字符串
 */
std::string queryDaemon(const UnixDomainSocket& socket, const std::string& request)
{
    std::cout << "[CLIENT] Sending request: \"" << request << "\"" << std::endl;
    
    // 发送请求
    auto sendResult = socket.send(request);
    if (!sendResult.has_value())
    {
        std::cerr << "[ERROR] Failed to send request" << std::endl;
        return "";
    }
    
    std::cout << "[CLIENT] ✅ Request sent, waiting for response..." << std::endl;
    
    // 接收响应
    std::string response;
    auto recvResult = socket.receive(response);
    if (!recvResult.has_value())
    {
        std::cerr << "[ERROR] Failed to receive response" << std::endl;
        return "";
    }
    
    std::cout << "[CLIENT] ✅ Received response: \"" << response << "\"" << std::endl;
    return response;
}

int main(int argc, char* argv[])
{
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Shared Memory Query Client (SOCK_DGRAM)                   ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    const char* socketPath = "/tmp/shm_daemon.sock";
    
    // 如果提供了命令行参数，使用自定义的套接字路径
    if (argc > 1)
    {
        socketPath = argv[1];
    }
    
    std::cout << "\n[CLIENT] Connecting to daemon at: " << socketPath << std::endl;
    
    // 创建客户端套接字（SOCK_DGRAM 模式）
    auto clientResult = UnixDomainSocketBuilder()
        .name(socketPath)
        .channelSide(PosixIpcChannelSide::CLIENT)
        .maxMsgSize(1024)
        .create();
    
    if (!clientResult.has_value())
    {
        std::cerr << "[ERROR] Failed to create client socket" << std::endl;
        std::cerr << "[INFO] Make sure the daemon server is running!" << std::endl;
        return 1;
    }
    
    auto& clientSocket = clientResult.value();
    std::cout << "[CLIENT] ✅ Client socket created successfully!" << std::endl;
    std::cout << "[CLIENT] Using SOCK_DGRAM (datagram) mode\n" << std::endl;
    
    // ==================== 场景 1: 获取共享内存路径 ====================
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Test 1: Get Shared Memory Path                            ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::string shmPath = queryDaemon(clientSocket, "GET_SHM_PATH");
    if (!shmPath.empty() && shmPath.find("ERROR") == std::string::npos)
    {
        std::cout << "[CLIENT] ✅ Success! Shared memory path: " << shmPath << std::endl;
        std::cout << "[CLIENT] Now I can use this path to access shared memory!\n" << std::endl;
    }
    else
    {
        std::cout << "[CLIENT] ❌ Failed to get shared memory path\n" << std::endl;
    }
    
    // ==================== 场景 2: 获取共享内存大小 ====================
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Test 2: Get Shared Memory Size                            ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::string shmSize = queryDaemon(clientSocket, "GET_SHM_SIZE");
    if (!shmSize.empty() && shmSize.find("ERROR") == std::string::npos)
    {
        std::cout << "[CLIENT] ✅ Success! Shared memory size: " << shmSize << " bytes\n" << std::endl;
    }
    else
    {
        std::cout << "[CLIENT] ❌ Failed to get shared memory size\n" << std::endl;
    }
    
    // ==================== 场景 3: Ping 测试 ====================
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Test 3: Ping Daemon                                       ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::string pingResponse = queryDaemon(clientSocket, "PING");
    if (pingResponse == "PONG")
    {
        std::cout << "[CLIENT] ✅ Daemon is alive and responding!\n" << std::endl;
    }
    else
    {
        std::cout << "[CLIENT] ❌ Unexpected ping response\n" << std::endl;
    }
    
    // ==================== 场景 4: 未知命令测试 ====================
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Test 4: Unknown Command                                   ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::string errorResponse = queryDaemon(clientSocket, "INVALID_COMMAND");
    if (errorResponse.find("ERROR") != std::string::npos)
    {
        std::cout << "[CLIENT] ✅ Daemon correctly rejected unknown command\n" << std::endl;
    }
    
    // ==================== 实际使用示例 ====================
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Example: How to use in real application                   ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::cout << "\n[EXAMPLE] Typical usage pattern:" << std::endl;
    std::cout << "\n// 1. 进程启动时查询共享内存地址" << std::endl;
    std::cout << "auto socket = UnixDomainSocketBuilder()" << std::endl;
    std::cout << "    .name(\"/tmp/shm_daemon.sock\")" << std::endl;
    std::cout << "    .channelSide(CLIENT)" << std::endl;
    std::cout << "    .create();" << std::endl;
    std::cout << "\nsocket.send(\"GET_SHM_PATH\");" << std::endl;
    std::cout << "std::string shmPath;" << std::endl;
    std::cout << "socket.receive(shmPath);" << std::endl;
    
    std::cout << "\n// 2. 使用获取到的路径打开共享内存" << std::endl;
    std::cout << "int shmFd = shm_open(shmPath.c_str(), O_RDWR, 0666);" << std::endl;
    std::cout << "void* addr = mmap(NULL, size, PROT_READ|PROT_WRITE," << std::endl;
    std::cout << "                  MAP_SHARED, shmFd, 0);" << std::endl;
    
    std::cout << "\n// 3. 进行零拷贝通信" << std::endl;
    std::cout << "memcpy(addr, data, dataSize);  // 写入数据" << std::endl;
    std::cout << "// 其他进程可以直接读取 addr 处的数据" << std::endl;
    
    std::cout << "\n[CLIENT] Done! All tests completed." << std::endl;
    
    // 套接字会在析构时自动关闭（RAII）
    
    return 0;
}

