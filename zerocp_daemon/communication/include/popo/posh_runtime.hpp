#ifndef POSH_RUNTIME_HPP
#define POSH_RUNTIME_HPP

#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include "runtime/ipc_interface_creator.hpp"
#include <memory>

namespace ZeroCP
{
namespace Runtime
{
// 使用与其他模块一致的 RuntimeName_t 定义
using RuntimeName_t = ZeroCP::string<108>;

/**
 * @class PoshRuntime
 * @brief 客户端运行时，类似iceoryx的PoshRuntime
 * 
 * 使用方式：
 * @code
 * auto& runtime = ZeroCP::Runtime::PoshRuntime::initRuntime(appName);
 * runtime.sendMessage("Hello");
 * @endcode
 */
class PoshRuntime
{
public:
    // 禁止拷贝和移动
    PoshRuntime(const PoshRuntime&) = delete;
    PoshRuntime& operator=(const PoshRuntime&) = delete;
    PoshRuntime(PoshRuntime&&) noexcept = delete;
    PoshRuntime& operator=(PoshRuntime&&) noexcept = delete;
    ~PoshRuntime() noexcept;
    
    /**
     * @brief 初始化运行时，自动连接守护进程并注册
     * @param runtimeName 应用程序名称
     * @return PoshRuntime& 运行时实例引用
     */
    static PoshRuntime& initRuntime(const RuntimeName_t& runtimeName) noexcept;
    
    /**
     * @brief 获取运行时实例
     * @return PoshRuntime& 运行时实例引用
     */
    static PoshRuntime& getInstance() noexcept;
    
    // 公共接口
    const RuntimeName_t& getRuntimeName() const noexcept;
    bool sendMessage(const std::string& message) noexcept;
    bool isConnected() const noexcept;
    
private:
    explicit PoshRuntime(const RuntimeName_t& runtimeName) noexcept;
    
    bool initializeConnection() noexcept;
    bool registerToRouteD() noexcept;
    bool receiveRouteDAck() noexcept;
    
    static PoshRuntime* m_instance;
    
    RuntimeName_t m_runtimeName;
    std::unique_ptr<IpcInterfaceCreator> m_ipcCreator;
    bool m_isConnected{false};
    uint32_t m_pid;
};

} // namespace Runtime
} // namespace ZeroCP

#endif // POSH_RUNTIME_HPP
