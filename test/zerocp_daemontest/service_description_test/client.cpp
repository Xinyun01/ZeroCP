#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include <unistd.h>

int main() {
    ZEROCP_LOG(Info, "ipc_client starting...");

    // 使用 PoshRuntime::initRuntime() 一步完成初始化和连接
    // 类似 iceoryx: iox::runtime::PoshRuntime::initRuntime("MyApp")
    ZeroCP::Runtime::RuntimeName_t appName;
    appName.insert(0, "MyClientApp");
    
    auto& runtime = ZeroCP::Runtime::PoshRuntime::initRuntime(appName);
    
    ZEROCP_LOG(Info, "PoshRuntime initialized successfully");
    ZEROCP_LOG(Info, "Runtime name: " << runtime.getRuntimeName().c_str());
    ZEROCP_LOG(Info, "Connected: " << (runtime.isConnected() ? "Yes" : "No"));
    
    // 发送测试消息
    if (runtime.isConnected()) {
        runtime.sendMessage("Hello from client application!");
        ZEROCP_LOG(Info, "Test message sent through PoshRuntime");
    }
    
    // 保持运行一段时间以便接收响应
    usleep(2000000);  // 2秒 = 2000000微秒
    
    ZEROCP_LOG(Info, "ipc_client done.");
    return 0;
}
