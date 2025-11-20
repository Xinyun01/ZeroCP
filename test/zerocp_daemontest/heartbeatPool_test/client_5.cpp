#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> g_keepRunning{true};

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        g_keepRunning.store(false);
    }
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    ZEROCP_LOG(Info, "Client_5 starting (PID: " << getpid() << ")");
    
    ZeroCP::Runtime::RuntimeName_t runtimeName;
    runtimeName.insert(0, "Client_5");
    auto& runtime = ZeroCP::Runtime::PoshRuntime::initRuntime(runtimeName);
    
    if (!runtime.isConnected())
    {
        ZEROCP_LOG(Error, "Failed to connect to daemon");
        return 1;
    }
    
    ZEROCP_LOG(Info, "Client_5 registered successfully");
    
    while (g_keepRunning.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    ZEROCP_LOG(Info, "Client_5 shutting down");
    return 0;
}
