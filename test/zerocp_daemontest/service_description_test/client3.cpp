#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <unistd.h>

int main() {
    ZEROCP_LOG(Info, "========== Client 3 Starting ==========");

    ZeroCP::Runtime::RuntimeName_t appName;
    appName.insert(0, "Client_3");
    
    auto& runtime = ZeroCP::Runtime::PoshRuntime::initRuntime(appName);
    
    ZEROCP_LOG(Info, "Client 3 initialized successfully");
    ZEROCP_LOG(Info, "Runtime name: " << runtime.getRuntimeName().c_str());
    ZEROCP_LOG(Info, "Connected: " << (runtime.isConnected() ? "Yes" : "No"));
    
    if (runtime.isConnected()) {
        runtime.sendMessage("Hello from Client 3!");
        ZEROCP_LOG(Info, "Message sent from Client 3");
    }
    
    usleep(5000000);  // 5秒
    
    ZEROCP_LOG(Info, "========== Client 3 Done ==========");
    return 0;
}
