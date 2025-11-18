#include <iostream>
#include <cstdlib>
#include <csignal>
#include <atomic>

#include "zerocp_daemon/diroute/diroute_memory_manager.hpp"
#include "zerocp_daemon/communication/include/diroute.hpp"

// 全局运行标志，用于信号处理
static std::atomic<bool> g_keepRunning{true};

// 信号处理函数，用于优雅关闭守护进程
void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\n[Signal] Received shutdown signal (" << signal << ")\n";
        g_keepRunning.store(false, std::memory_order_release);
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    std::cout << "=== Diroute Daemon: Starting ===\n\n";

    // 设置信号处理器，用于优雅关闭守护进程
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::cout << "[Main] Signal handlers registered (SIGINT, SIGTERM)\n\n";

    // 创建并初始化共享内存池（iceoryx 静态构造模式）
    std::cout << "[Main] Creating memory pool...\n";
    
    auto memoryManagerResult = ZeroCP::Diroute::DirouteMemoryManager::createMemoryPool();
    
    if (!memoryManagerResult)
    {
        std::cerr << "[Main Error] Failed to create memory pool\n";
        return EXIT_FAILURE;
    }
    
    auto memoryManager = std::move(*memoryManagerResult);
    std::cout << "[Main] Memory pool initialized: " 
              << (memoryManager.isInitialized() ? "YES" : "NO") << "\n\n";

    // 为守护进程注册心跳槽位（守护进程持有此槽位证明自己存活）
    auto& heartbeatPool = memoryManager.getHeartbeatPool();
    auto daemonSlot = heartbeatPool.emplace();
    if (daemonSlot == heartbeatPool.end())
    {
        std::cerr << "[Main Error] Failed to emplace daemon heartbeat slot\n";
        return EXIT_FAILURE;
    }
    
    // touch() 作用：
    // 1. 获取当前时间点 std::chrono::steady_clock::now()
    // 2. 转换为纳秒（距离 epoch 的纳秒数）
    // 3. 原子地存储到 HeartbeatSlot 的 m_lastTimestamp 中
    // 4. 其他进程可以通过读取此时间戳判断守护进程是否存活
    daemonSlot->touch();
    std::cout << "[Main] Daemon heartbeat slot registered (will be held during runtime)\n";
    std::cout << "[Main] Initial timestamp: " << daemonSlot->load() << " ns\n\n";

    // 创建并启动 Diroute（多线程监控与路由）
    std::cout << "[Main] Starting Diroute monitoring and routing...\n";
    ZeroCP::Diroute::Diroute diroute;
    diroute.run();
    std::cout << "[Main] Diroute started (multi-threaded)\n\n";

    // 守护进程主循环 - 等待信号并定期更新心跳
    std::cout << "=== Daemon Running ===\n";
    std::cout << "[Daemon] Shared memory: /zerocp_diroute_components\n";
    std::cout << "[Daemon] Press Ctrl+C to shutdown gracefully\n\n";

    constexpr int HEARTBEAT_INTERVAL_MS = 1000;  // 每 1 秒更新一次心跳
    int loopCount = 0;

    while (g_keepRunning.load(std::memory_order_acquire))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 每秒更新一次守护进程的心跳时间戳
        // 外部监控工具可以通过检查此时间戳来判断守护进程是否挂起或崩溃
        if (++loopCount >= (HEARTBEAT_INTERVAL_MS / 100))
        {
            daemonSlot->touch();  // 写入最新的纳秒级时间戳到共享内存
            loopCount = 0;
        }
    }

    // 优雅关闭流程
    std::cout << "\n[Daemon] Initiating graceful shutdown...\n";
    
    // 停止 Diroute 的所有工作线程
    std::cout << "[Daemon] Stopping Diroute threads...\n";
    diroute.stop();
    std::cout << "[Daemon] Diroute stopped\n";

    // 释放守护进程的心跳槽位（归还到槽位池）
    std::cout << "[Daemon] Releasing daemon heartbeat slot...\n";
    heartbeatPool.release(daemonSlot);
    std::cout << "[Daemon] Daemon heartbeat slot released\n";

    // 通过 RAII 自动清理资源
    std::cout << "[Daemon] Cleaning up resources:\n";
    std::cout << "[Daemon]   - Destroying HeartbeatPool\n";
    std::cout << "[Daemon]   - Unmapping shared memory\n";
    std::cout << "[Daemon]   - Closing file descriptors\n";

    std::cout << "\n=== Diroute Daemon: Stopped ===\n";
    
    return EXIT_SUCCESS;
}

