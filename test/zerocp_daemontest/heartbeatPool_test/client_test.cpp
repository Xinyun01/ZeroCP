#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

std::atomic<bool> g_keepRunning{true};

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        ZEROCP_LOG(Info, "Received signal, shutting down...");
        g_keepRunning.store(false);
    }
}

int main(int argc, char* argv[])
{
    // 安装信号处理器
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // 从命令行参数获取客户端ID
    std::string clientId = "Client_0";
    if (argc > 1)
    {
        clientId = argv[1];
    }
    
    ZEROCP_LOG(Info, "======================================");
    ZEROCP_LOG(Info, "Starting Heartbeat Client Test");
    ZEROCP_LOG(Info, "Client ID: " << clientId);
    ZEROCP_LOG(Info, "PID: " << getpid());
    ZEROCP_LOG(Info, "======================================");
    
    // 初始化 PoshRuntime（自动注册到守护进程）
    ZeroCP::Runtime::RuntimeName_t runtimeName;
    runtimeName.insert(0, clientId.c_str());
    
    auto& runtime = ZeroCP::Runtime::PoshRuntime::initRuntime(runtimeName);
    
    if (!runtime.isConnected())
    {
        ZEROCP_LOG(Error, "Failed to connect to daemon");
        return 1;
    }
    
    ZEROCP_LOG(Info, "✓ Successfully registered to daemon");
    ZEROCP_LOG(Info, "✓ Heartbeat thread is running (100ms interval)");
    ZEROCP_LOG(Info, "Press Ctrl+C to exit gracefully");
    
    // 主循环 - 等待信号
    int counter = 0;
    while (g_keepRunning.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        counter++;
        ZEROCP_LOG(Info, "[" << clientId << "] Running... (" << (counter * 5) << "s)");
    }
    
    ZEROCP_LOG(Info, "Client shutting down gracefully...");
    // PoshRuntime 析构时会自动停止心跳线程
    
    return 0;
}
