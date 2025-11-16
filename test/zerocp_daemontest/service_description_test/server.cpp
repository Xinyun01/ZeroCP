#include "diroute.hpp"
#include "runtime/process_manager.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <csignal>
#include <atomic>
#include <thread>
#include <unistd.h>  // 使用 usleep 替代 chrono 避开 GCC 13 bug

static std::atomic<bool> g_running{true};

static void handleSignal(int) {
    g_running = false;
}

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    ZEROCP_LOG(Info, "========================================");
    ZEROCP_LOG(Info, "Diroute Server Starting...");
    ZEROCP_LOG(Info, "========================================");
    
    ZeroCP::Diroute::Diroute server;
    server.run();

    // 刷新计数器：每5秒刷新一次（5000ms / 200ms = 25次循环）
    int refreshCounter = 0;
    const int REFRESH_INTERVAL = 25;  // 每25次循环刷新一次（约5秒）

    while (g_running.load()) {
        usleep(200000);  // 200ms = 200000 微秒（避开 GCC 13 chrono bug）
        
        refreshCounter++;
        
        // 定期刷新显示当前连接的客户端
        if (refreshCounter >= REFRESH_INTERVAL) {
            refreshCounter = 0;  // 重置计数器
            
            ZEROCP_LOG(Info, "");
            ZEROCP_LOG(Info, "╔════════════════════════════════════════════════════════════╗");
            ZEROCP_LOG(Info, "║         Current Connected Clients to Diroute Server       ║");
            ZEROCP_LOG(Info, "╚════════════════════════════════════════════════════════════╝");
            
            auto& processMgr = ZeroCP::Runtime::ProcessManager::getInstance();
            processMgr.printAllProcesses();
            
            ZEROCP_LOG(Info, "");
        }
    }

    ZEROCP_LOG(Info, "========================================");
    ZEROCP_LOG(Info, "Diroute Server Stopping...");
    ZEROCP_LOG(Info, "========================================");
    server.stop();
    ZEROCP_LOG(Info, "Diroute Server Exited.");
    return 0;
}


