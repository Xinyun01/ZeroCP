#ifndef ZEROCP_DIROUTE_HPP
#define ZEROCP_DIROUTE_HPP

#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include "zerocp_foundationLib/posix/memory/include/unix_domainsocket.hpp"
#include "runtime/ipc_interface_creator.hpp"
#include "runtime/process_manager.hpp"
#include "runtime/message_runtime.hpp"
#include "service_description.hpp"
#include "popo/message_header.hpp"
#include <thread>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <set>
#include <optional>
namespace ZeroCP
{
// RuntimeName_t 已在 process_manager.hpp 中定义为 string<108>
namespace Diroute
{

// 前向声明
class DirouteMemoryManager;

using IpcInterfaceCreator_t = ZeroCP::Runtime::IpcInterfaceCreator;
using ProcessManager = ZeroCP::Runtime::ProcessManager;
using RuntimeName_t = ZeroCP::Runtime::RuntimeName_t;

class Diroute
{
public:
    explicit Diroute(DirouteMemoryManager* memoryManager) noexcept;
    Diroute() = delete;
    Diroute(const Diroute& other) = delete;
    Diroute(Diroute&& other) noexcept = delete;
    Diroute& operator=(const Diroute& other) = delete;
    Diroute& operator=(Diroute&& other) noexcept = delete;
    ~Diroute() noexcept;
    void run() noexcept;
    void stop() noexcept;
    void startProcessRuntimeMessagesThread() noexcept;
    void processRuntimeMessagesThread() noexcept;
    void registerProcess(RuntimeName_t m_Runtime) noexcept;
    
    size_t getRegisteredProcessCount() const noexcept;
    void printRegisteredProcesses() const noexcept;
    
private:
    struct ProcessInfo
    {
        std::string name;
        uint32_t pid;
        uint64_t slotIndex;
    };
    
    /// @brief Publisher 注册信息
    struct PublisherInfo
    {
        RuntimeName_t processName;      // 发布者进程名称
        ServiceDescription serviceDesc; // 服务描述（service, instance, event）
        uint64_t slotIndex;              // 心跳槽位索引
        uint32_t pid;                   // 进程 ID
        
        PublisherInfo(const RuntimeName_t& name, const ServiceDescription& desc, 
                     uint64_t slot, uint32_t processId) noexcept
            : processName(name), serviceDesc(desc), slotIndex(slot), pid(processId)
        {
        }
    };
    
    /// @brief Subscriber 注册信息
    struct SubscriberInfo
    {
        RuntimeName_t processName;      // 订阅者进程名称
        ServiceDescription serviceDesc; // 服务描述（service, instance, event）
        uint64_t slotIndex;              // 心跳槽位索引
        uint32_t queueIndex;             // 接收队列索引
        uint64_t receiveQueueOffset;     // 接收队列在共享内存中的偏移量
        uint32_t pid;                   // 进程 ID
        
        SubscriberInfo(const RuntimeName_t& name, const ServiceDescription& desc,
                      uint64_t slot, uint32_t queueIdx, uint64_t queueOffset, uint32_t processId) noexcept
            : processName(name)
            , serviceDesc(desc)
            , slotIndex(slot)
            , queueIndex(queueIdx)
            , receiveQueueOffset(queueOffset)
            , pid(processId)
        {
        }
    };
    
    void handleProcessRegistration(const ZeroCP::Runtime::RuntimeMessage& message,
                                    ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept;
    
    /// @brief 处理 Publisher 注册
    /// @param message 格式: "PUBLISHER:<processName>:<pid>:<service>:<instance>:<event>"
    void handlePublisherRegistration(const ZeroCP::Runtime::RuntimeMessage& message,
                                     ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept;
    
    /// @brief 处理 Subscriber 注册
    /// @param message 格式: "SUBSCRIBER:<processName>:<pid>:<service>:<instance>:<event>"
    void handleSubscriberRegistration(const ZeroCP::Runtime::RuntimeMessage& message,
                                      ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept;
    
    /// @brief 处理消息路由（从 Publisher 到 Subscriber）
    /// @param message 格式: "ROUTE:<slotIndex>:<service>:<instance>:<event>:<poolId>:<chunkOffset>"
    void handleMessageRouting(const ZeroCP::Runtime::RuntimeMessage& message,
                              ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept;
    
    /// @brief 匹配 Publisher 和 Subscriber
    /// @param serviceDesc 服务描述
    /// @return 匹配的 Subscriber 列表
    std::vector<SubscriberInfo*> matchSubscribers(const ServiceDescription& serviceDesc) noexcept;
    
    /// @brief 将消息路由到订阅者的接收队列
    /// @param subscriber 订阅者信息
    /// @param chunk Chunk 句柄
    /// @param publisherName 发布者名称
    /// @return 成功返回 true
    bool routeMessageToSubscriber(const SubscriberInfo& subscriber,
                                  const Popo::ChunkHandle& chunk,
                                  const RuntimeName_t& publisherName) noexcept;

    /// @brief 通过 slotIndex 查找 Publisher 名称
    [[nodiscard]] std::optional<RuntimeName_t> findPublisherName(uint64_t slotIndex,
                                                                 const ServiceDescription& desc) const noexcept;
    
    void startHeartbeatMonitorThread() noexcept;
    void heartbeatMonitorThreadFunc() noexcept;
    void checkHeartbeatTimeouts() noexcept;
    
    /// @brief 清理已死亡进程的 Publisher/Subscriber 注册
    void cleanupDeadProcessRegistrations(uint64_t slotIndex) noexcept;
    
    DirouteMemoryManager* m_memoryManager{nullptr};
    std::thread m_startProcessRuntimeMessagesThread;
    std::thread m_heartbeatMonitorThread;
    std::atomic<bool> m_runMonitoringAndDiscoveryThread{false};
    
    // 进程注册信息
    std::unordered_map<uint64_t, ProcessInfo> m_registeredProcesses;
    mutable std::mutex m_processesMutex;
    
    // Publisher/Subscriber 注册信息
    std::vector<PublisherInfo> m_publishers;
    std::vector<SubscriberInfo> m_subscribers;
    mutable std::mutex m_pubSubMutex;
    
    // 序列号生成器（用于消息去重）
    std::atomic<uint64_t> m_sequenceNumber{0};
};
} // namespace Diroute

} // namespace ZeroCP
#endif
