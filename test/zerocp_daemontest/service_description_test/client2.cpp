#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <unistd.h>

int main() {
    ZEROCP_LOG(Info, "========== Client 2 Starting ==========");

    // 初始化运行时
    ZeroCP::Runtime::RuntimeName_t appName;
    appName.insert(0, "Client_2");
    
    auto& runtime = ZeroCP::Runtime::PoshRuntime::initRuntime(appName);
    
    ZEROCP_LOG(Info, "Client 2 initialized successfully");
    ZEROCP_LOG(Info, "Runtime name: " << runtime.getRuntimeName().c_str());
    ZEROCP_LOG(Info, "Connected: " << (runtime.isConnected() ? "Yes" : "No"));
    
    // 发送测试消息
    if (runtime.isConnected()) {
        runtime.sendMessage("Hello from Client 2!");
        ZEROCP_LOG(Info, "Message sent from Client 2");
    }
    
    // 保持运行
    usleep(5000000);  // 5秒
    
    ZEROCP_LOG(Info, "========== Client 2 Done ==========");
    return 0;
}
