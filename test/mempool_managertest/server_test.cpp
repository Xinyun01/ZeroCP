#include "diroute.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> g_running{true};

static void handleSignal(int) {
    g_running = false;
}

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    ZEROCP_LOG(Info, "server_test starting Diroute...");
    ZeroCP::Diroute::Diroute server;
    server.run();

    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    ZEROCP_LOG(Info, "server_test stopping Diroute...");
    server.stop();
    ZEROCP_LOG(Info, "server_test exited.");
    return 0;
}


