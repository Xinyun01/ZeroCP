#include "zerocp_foundationLib/report/include/logging.hpp"
#include "diroute.hpp"
#include "zerocp_daemon/diroute/diroute_memory_manager.hpp"
#include "runtime/ipc_interface_creator.hpp"
#include "runtime/message_runtime.hpp"
#include "popo/message_header.hpp"
#include "zerocp_foundationLib/report/include/lockfree_ringbuffer.hpp"
#include <thread>
#include <sstream>
#include <unistd.h>
#include <chrono>
#include <algorithm>

namespace ZeroCP
{
namespace Diroute
{

Diroute::Diroute(DirouteMemoryManager* memoryManager) noexcept
    : m_memoryManager(memoryManager)
{
}

void Diroute::run() noexcept
{
    m_runMonitoringAndDiscoveryThread = true;
    startProcessRuntimeMessagesThread();
    startHeartbeatMonitorThread();
}
    
Diroute::~Diroute() noexcept
{
    stop();
}

void Diroute::stop() noexcept
{
    ZEROCP_LOG(Info, "Stopping Diroute threads...");
    m_runMonitoringAndDiscoveryThread = false;
    
    // 给线程一点时间来检测停止标志并退出
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (m_startProcessRuntimeMessagesThread.joinable())
    {
        ZEROCP_LOG(Info, "Waiting for runtime messages thread to join...");
        m_startProcessRuntimeMessagesThread.join();
        ZEROCP_LOG(Info, "Runtime messages thread joined");
    }
    
    if (m_heartbeatMonitorThread.joinable())
    {
        ZEROCP_LOG(Info, "Waiting for heartbeat monitor thread to join...");
        m_heartbeatMonitorThread.join();
        ZEROCP_LOG(Info, "Heartbeat monitor thread joined");
    }
    
    ZEROCP_LOG(Info, "All Diroute threads stopped");
}
// 启动进程运行时消息处理线程    
void Diroute::startProcessRuntimeMessagesThread() noexcept
{
    m_startProcessRuntimeMessagesThread = std::thread(&Diroute::processRuntimeMessagesThread, this);
}

/// 进程运行时消息处理线程主循环
void Diroute::processRuntimeMessagesThread() noexcept
{
    // 在工作线程中创建并绑定服务端 UDS
    ZeroCP::Runtime::IpcInterfaceCreator creator;
    ZeroCP::Runtime::RuntimeName_t serverName;
    serverName.insert(0, "udsServer");
    // UDS会自动添加前导"/"，使用相对路径即可
    const char* socketPath = "udsServer.sock";
    ZEROCP_LOG(Info, "Creating server UDS at: " << socketPath);
    auto udsRes = creator.createUnixDomainSocket(serverName, ZeroCP::PosixIpcChannelSide::SERVER, socketPath);
    if (!udsRes.has_value())
    {
        ZEROCP_LOG(Error, "Failed to create server UDS in runtime thread.");
        return;
    }
    while(m_runMonitoringAndDiscoveryThread)
    {
        ZeroCP::Runtime::RuntimeMessage message;
        auto receiveRes = creator.receiveMessage(message);
        if (!receiveRes)
        {
            // 检查是否是因为停止标志而退出
            if (!m_runMonitoringAndDiscoveryThread)
            {
                break;
            }
            // 其他错误，短暂休眠后继续
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        ZEROCP_LOG(Info, "Received message in runtime thread: " << message.c_str());
        
        // 根据消息类型路由到不同的处理函数
        std::istringstream iss(message);
        std::string command;
        if (std::getline(iss, command, ':'))
        {
            if (command == "REGISTER")
            {
                // 处理进程注册消息
                handleProcessRegistration(message, creator);
            }
            else if (command == "PUBLISHER")
            {
                // 处理 Publisher 注册
                handlePublisherRegistration(message, creator);
            }
            else if (command == "SUBSCRIBER")
            {
                // 处理 Subscriber 注册
                handleSubscriberRegistration(message, creator);
            }
            else if (command == "ROUTE")
            {
                // 处理消息路由
                handleMessageRouting(message, creator);
            }
            else
            {
                ZEROCP_LOG(Warn, "Unknown command: " << command);
                ZeroCP::Runtime::RuntimeMessage response = "ERROR:UNKNOWN_COMMAND";
                creator.sendMessage(response);
            }
        }
        else
        {
            ZEROCP_LOG(Warn, "Invalid message format (no command): " << message);
            ZeroCP::Runtime::RuntimeMessage response = "ERROR:INVALID_FORMAT";
            creator.sendMessage(response);
        }
    }
}

/// 处理进程注册消息
void Diroute::handleProcessRegistration(const ZeroCP::Runtime::RuntimeMessage& message,
                                        ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept
{
    if (!m_memoryManager)
    {
        ZEROCP_LOG(Error, "MemoryManager not initialized");
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:MEMORY_NOT_INITIALIZED";
        creator.sendMessage(response);
        return;
    }

    // 解析消息："REGISTER:<processName>:<pid>:<isMonitored>"
    std::istringstream iss(message);
    std::string command, processName, pidStr, monitoredStr;
    
    if (!std::getline(iss, command, ':') || command != "REGISTER")
    {
        ZEROCP_LOG(Warn, "Invalid message format: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:INVALID_FORMAT";
        creator.sendMessage(response);
        return;
    }
    
    if (!std::getline(iss, processName, ':') || 
        !std::getline(iss, pidStr, ':') || 
        !std::getline(iss, monitoredStr))
    {
        ZEROCP_LOG(Error, "Failed to parse message: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:PARSE_FAILED";
        creator.sendMessage(response);
        return;
    }
    
    // 转换 PID
    uint32_t pid = 0;
    try
    {
        pid = std::stoul(pidStr);
    }
    catch (const std::exception& e)
    {
        ZEROCP_LOG(Error, "Invalid PID: " << pidStr);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:INVALID_PID";
        creator.sendMessage(response);
        return;
    }
    
    // 从 HeartbeatPool 分配槽位
    auto& heartbeatPool = m_memoryManager->getHeartbeatPool();
    
    // 检查槽位池是否已满
    if (heartbeatPool.isFull())
    {
        ZEROCP_LOG(Error, "Heartbeat pool is full, cannot register: " << processName);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:POOL_FULL";
        creator.sendMessage(response);
        return;
    }
    
    // 分配槽位
    auto slotIt = heartbeatPool.emplace();
    if (slotIt == heartbeatPool.end())
    {
        ZEROCP_LOG(Error, "Failed to allocate heartbeat slot for: " << processName);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:ALLOCATION_FAILED";
        creator.sendMessage(response);
        return;
    }
    
    // 立即初始化心跳时间戳，避免 lastHeartbeat == 0 的情况
    slotIt->touch();
    
    const uint64_t slotIndex = slotIt.to_index();
    
    ZEROCP_LOG(Info, "Registered process: " << processName 
               << " (PID: " << pid << ") with heartbeat slot index: " << slotIndex);
    
    // 发送响应："OK:OFFSET:<slotIndex>"
    std::ostringstream responseStream;
    responseStream << "OK:OFFSET:" << slotIndex;
    ZeroCP::Runtime::RuntimeMessage response = responseStream.str();
    
    if (!creator.sendMessage(response))
    {
        ZEROCP_LOG(Error, "Failed to send response to: " << processName);
        // 回滚：释放槽位
        heartbeatPool.release(slotIt);
        return;
    }
    
    ZEROCP_LOG(Info, "✓ Sent slot index to " << processName << ": " << slotIndex);
    
    // 记录进程信息
    {
        std::lock_guard<std::mutex> lock(m_processesMutex);
        m_registeredProcesses[slotIndex] = ProcessInfo{processName, pid, slotIndex};
        ZEROCP_LOG(Info, "✓ Total registered processes: " << m_registeredProcesses.size());
    }
}

void Diroute::startHeartbeatMonitorThread() noexcept
{
    m_heartbeatMonitorThread = std::thread(&Diroute::heartbeatMonitorThreadFunc, this);
    ZEROCP_LOG(Info, "Heartbeat monitor thread started");
}

void Diroute::heartbeatMonitorThreadFunc() noexcept
{
    uint32_t checkCount = 0;
    while (m_runMonitoringAndDiscoveryThread)
    {
        // 每 300ms 检查一次心跳超时
        checkHeartbeatTimeouts();
        
        // 每 1 秒打印一次注册进程列表（300ms * 3 ≈ 1秒）
        if (checkCount % 3 == 0)
        {
            printRegisteredProcesses();
        }
        checkCount++;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    ZEROCP_LOG(Info, "Heartbeat monitor thread stopped");
}

size_t Diroute::getRegisteredProcessCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_processesMutex);
    return m_registeredProcesses.size();
}

void Diroute::printRegisteredProcesses() const noexcept
{
    std::lock_guard<std::mutex> lock(m_processesMutex);
    
    ZEROCP_LOG(Info, "========================================");
    ZEROCP_LOG(Info, "Registered Processes: " << m_registeredProcesses.size());
    ZEROCP_LOG(Info, "========================================");
    
    if (m_registeredProcesses.empty())
    {
        ZEROCP_LOG(Info, "  (No processes registered)");
    }
    else
    {
        for (const auto& [slotIndex, processInfo] : m_registeredProcesses)
        {
            ZEROCP_LOG(Info, "  [" << slotIndex << "] " 
                       << processInfo.name 
                       << " (PID: " << processInfo.pid << ")");
        }
    }
    ZEROCP_LOG(Info, "========================================");
}

// ============================================================================
// 心跳超时检测与应用进程清理
// ============================================================================
// 功能：
//   1. 每300ms被调用一次（在heartbeatMonitorThreadFunc中）
//   2. 获取当前绝对时间，与共享内存中的心跳时间对比
//   3. 如果时间差超过3秒，则判定为超时
//   4. 删除超时应用进程的注册信息，释放心跳槽位
// ============================================================================
void Diroute::checkHeartbeatTimeouts() noexcept
{
    if (!m_memoryManager)
    {
        return;
    }
    
    auto& heartbeatPool = m_memoryManager->getHeartbeatPool();
    
    // ===== 步骤1: 获取当前绝对时间 =====
    auto now = std::chrono::steady_clock::now();
    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    // ===== 步骤2: 定义超时阈值（3秒 = 3,000,000,000纳秒）=====
    // 参考 heartbeatPool_test/README.md：守护进程需在 3 秒内回收死进程的槽位
    const uint64_t TIMEOUT_NS = 3'000'000'000ULL;  // 3 秒超时阈值
    
    std::lock_guard<std::mutex> lock(m_processesMutex);
    
    // 存储超时的进程offset
    std::vector<uint64_t> timeoutProcesses;
    
    // ===== 步骤3: 遍历所有注册的应用进程，检查心跳时间 =====
    for (const auto& [slotIndex, processInfo] : m_registeredProcesses)
    {
        auto it = heartbeatPool.iteratorFromIndex(slotIndex);
        
        if (it != heartbeatPool.end())
        {
            // 从共享内存读取应用进程最后一次写入的心跳时间
            uint64_t lastHeartbeat = it->load();
            
            // 计算时间差（心跳年龄）
            uint64_t age_ns = (lastHeartbeat == 0) ? 0 : (now_ns - lastHeartbeat);
            
            // 打印心跳检查信息（调试用）
            ZEROCP_LOG(Info, "[HeartbeatCheck] " << processInfo.name 
                       << " (PID: " << processInfo.pid 
                       << ", slotIndex: " << slotIndex 
                       << ") lastHB=" << lastHeartbeat 
                       << " age=" << (age_ns / 1'000'000) << "ms");
            
            // 如果心跳为0，跳过检查（可能刚注册尚未更新）
            if (lastHeartbeat == 0)
            {
                ZEROCP_LOG(Warn, "Process " << processInfo.name 
                           << " (PID: " << processInfo.pid 
                           << ", slotIndex: " << slotIndex
                           << ") has ZERO heartbeat timestamp - skipping check!");
                continue;
            }
            
            // ===== 步骤4: 判断是否超时（时间差 > 3秒）=====
            if (age_ns > TIMEOUT_NS)
            {
                ZEROCP_LOG(Warn, "⚠️  Process timeout detected: " << processInfo.name 
                           << " (PID: " << processInfo.pid 
                           << ", slotIndex: " << slotIndex
                           << ", age: " << (age_ns / 1'000'000) << "ms)");
                
                // 记录超时的进程offset，稍后统一删除
                timeoutProcesses.push_back(slotIndex);
            }
        }
    }
    
    // ===== 步骤5: 删除所有超时的应用进程注册信息 =====
    // 【这里是删除应用进程的关键位置】
    for (uint64_t slotIndex : timeoutProcesses)
    {
        auto processIt = m_registeredProcesses.find(slotIndex);
        if (processIt != m_registeredProcesses.end())
        {
            const auto removedProcess = processIt->second;
            ZEROCP_LOG(Info, "🗑️  Releasing slot for dead process: " << removedProcess.name
                       << " (slotIndex: " << slotIndex << ")");
            
            // 5.1 释放心跳槽位（共享内存）
            auto slotIt = heartbeatPool.iteratorFromIndex(slotIndex);
            heartbeatPool.release(slotIt);
            
            // 5.2 从注册列表中删除该应用进程
            // 【关键操作：删除应用进程的注册信息】
            m_registeredProcesses.erase(processIt);
            
            ZEROCP_LOG(Info, "✅ Process " << removedProcess.name << " removed from registry");
            ZEROCP_LOG(Info, "✓ Total registered processes: " << m_registeredProcesses.size());
        }
    }
    
    // 打印清理结果
    if (!timeoutProcesses.empty())
    {
        ZEROCP_LOG(Info, "✓ Cleanup completed. Remaining registered processes: " 
                   << m_registeredProcesses.size());
        
        // 清理已死亡进程的 Publisher/Subscriber 注册
        for (uint64_t slotIndex : timeoutProcesses)
        {
            cleanupDeadProcessRegistrations(slotIndex);
        }
    }
}

// ============================================================================
// Publisher/Subscriber 注册与匹配机制
// ============================================================================

/// 处理 Publisher 注册
/// 消息格式: "PUBLISHER:<processName>:<pid>:<service>:<instance>:<event>"
void Diroute::handlePublisherRegistration(const ZeroCP::Runtime::RuntimeMessage& message,
                                          ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept
{
    if (!m_memoryManager)
    {
        ZEROCP_LOG(Error, "MemoryManager not initialized");
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:MEMORY_NOT_INITIALIZED";
        creator.sendMessage(response);
        return;
    }
    
    // 解析消息
    std::istringstream iss(message);
    std::string command, processName, pidStr, service, instance, event;
    
    if (!std::getline(iss, command, ':') || command != "PUBLISHER")
    {
        ZEROCP_LOG(Warn, "Invalid PUBLISHER message format: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:INVALID_FORMAT";
        creator.sendMessage(response);
        return;
    }
    
    if (!std::getline(iss, processName, ':') ||
        !std::getline(iss, pidStr, ':') ||
        !std::getline(iss, service, ':') ||
        !std::getline(iss, instance, ':') ||
        !std::getline(iss, event))
    {
        ZEROCP_LOG(Error, "Failed to parse PUBLISHER message: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:PARSE_FAILED";
        creator.sendMessage(response);
        return;
    }
    
    // 转换 PID
    uint32_t pid = 0;
    try
    {
        pid = std::stoul(pidStr);
    }
    catch (const std::exception& e)
    {
        ZEROCP_LOG(Error, "Invalid PID: " << pidStr);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:INVALID_PID";
        creator.sendMessage(response);
        return;
    }
    
    // 查找进程的心跳槽位
    uint64_t slotIndex = 0;
    {
        std::lock_guard<std::mutex> lock(m_processesMutex);
        bool found = false;
        for (const auto& [idx, procInfo] : m_registeredProcesses)
        {
            if (procInfo.name == processName && procInfo.pid == pid)
            {
                slotIndex = idx;
                found = true;
                break;
            }
        }
        if (!found)
        {
            ZEROCP_LOG(Error, "Process not registered: " << processName);
            ZeroCP::Runtime::RuntimeMessage response = "ERROR:PROCESS_NOT_REGISTERED";
            creator.sendMessage(response);
            return;
        }
    }
    
    // 创建 ServiceDescription
    ZeroCP::id_string serviceStr, instanceStr, eventStr;
    serviceStr.insert(0, service.c_str());
    instanceStr.insert(0, instance.c_str());
    eventStr.insert(0, event.c_str());
    ServiceDescription serviceDesc(serviceStr, instanceStr, eventStr);
    
    // 注册 Publisher
    {
        std::lock_guard<std::mutex> lock(m_pubSubMutex);
        RuntimeName_t runtimeName;
        runtimeName.insert(0, processName.c_str());
        
        // 检查是否已注册
        bool alreadyRegistered = false;
        for (const auto& pub : m_publishers)
        {
            if (pub.processName == runtimeName && pub.serviceDesc == serviceDesc)
            {
                alreadyRegistered = true;
                break;
            }
        }
        
        if (!alreadyRegistered)
        {
            m_publishers.emplace_back(runtimeName, serviceDesc, slotIndex, pid);
            ZEROCP_LOG(Info, "✓ Registered Publisher: " << processName 
                      << " -> " << service << "/" << instance << "/" << event);
        }
        else
        {
            ZEROCP_LOG(Warn, "Publisher already registered: " << processName);
        }
    }
    
    // 发送成功响应
    ZeroCP::Runtime::RuntimeMessage response = "OK:PUBLISHER_REGISTERED";
    creator.sendMessage(response);
}

/// 处理 Subscriber 注册
/// 消息格式: "SUBSCRIBER:<processName>:<pid>:<service>:<instance>:<event>"
void Diroute::handleSubscriberRegistration(const ZeroCP::Runtime::RuntimeMessage& message,
                                           ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept
{
    if (!m_memoryManager)
    {
        ZEROCP_LOG(Error, "MemoryManager not initialized");
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:MEMORY_NOT_INITIALIZED";
        creator.sendMessage(response);
        return;
    }
    
    // 解析消息
    std::istringstream iss(message);
    std::string command, processName, pidStr, service, instance, event;
    
    if (!std::getline(iss, command, ':') || command != "SUBSCRIBER")
    {
        ZEROCP_LOG(Warn, "Invalid SUBSCRIBER message format: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:INVALID_FORMAT";
        creator.sendMessage(response);
        return;
    }
    
    if (!std::getline(iss, processName, ':') ||
        !std::getline(iss, pidStr, ':') ||
        !std::getline(iss, service, ':') ||
        !std::getline(iss, instance, ':') ||
        !std::getline(iss, event))
    {
        ZEROCP_LOG(Error, "Failed to parse SUBSCRIBER message: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:PARSE_FAILED";
        creator.sendMessage(response);
        return;
    }
    
    // 转换 PID
    uint32_t pid = 0;
    try
    {
        pid = std::stoul(pidStr);
    }
    catch (const std::exception& e)
    {
        ZEROCP_LOG(Error, "Invalid PID: " << pidStr);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:INVALID_PID";
        creator.sendMessage(response);
        return;
    }
    
    // 查找进程的心跳槽位
    uint64_t slotIndex = 0;
    {
        std::lock_guard<std::mutex> lock(m_processesMutex);
        bool found = false;
        for (const auto& [idx, procInfo] : m_registeredProcesses)
        {
            if (procInfo.name == processName && procInfo.pid == pid)
            {
                slotIndex = idx;
                found = true;
                break;
            }
        }
        if (!found)
        {
            ZEROCP_LOG(Error, "Process not registered: " << processName);
            ZeroCP::Runtime::RuntimeMessage response = "ERROR:PROCESS_NOT_REGISTERED";
            creator.sendMessage(response);
            return;
        }
    }
    
    // 创建 ServiceDescription
    ZeroCP::id_string serviceStr, instanceStr, eventStr;
    serviceStr.insert(0, service.c_str());
    instanceStr.insert(0, instance.c_str());
    eventStr.insert(0, event.c_str());
    ServiceDescription serviceDesc(serviceStr, instanceStr, eventStr);
    
    // TODO: 在共享内存中为 Subscriber 分配接收队列
    // 这里暂时使用 slotIndex 作为队列偏移量的占位符
    // 实际实现中，应该在 DirouteComponents 中管理接收队列
    uint64_t receiveQueueOffset = slotIndex * 1024; // 临时方案：每个槽位分配 1KB 队列空间
    
    // 注册 Subscriber
    {
        std::lock_guard<std::mutex> lock(m_pubSubMutex);
        RuntimeName_t runtimeName;
        runtimeName.insert(0, processName.c_str());
        
        // 检查是否已注册
        bool alreadyRegistered = false;
        for (const auto& sub : m_subscribers)
        {
            if (sub.processName == runtimeName && sub.serviceDesc == serviceDesc)
            {
                alreadyRegistered = true;
                break;
            }
        }
        
        if (!alreadyRegistered)
        {
            m_subscribers.emplace_back(runtimeName, serviceDesc, slotIndex, receiveQueueOffset, pid);
            ZEROCP_LOG(Info, "✓ Registered Subscriber: " << processName 
                      << " -> " << service << "/" << instance << "/" << event
                      << " (queueOffset: " << receiveQueueOffset << ")");
        }
        else
        {
            ZEROCP_LOG(Warn, "Subscriber already registered: " << processName);
        }
    }
    
    // 发送成功响应（包含队列偏移量）
    std::ostringstream responseStream;
    responseStream << "OK:SUBSCRIBER_REGISTERED:QUEUE_OFFSET:" << receiveQueueOffset;
    ZeroCP::Runtime::RuntimeMessage response = responseStream.str();
    creator.sendMessage(response);
}

/// 匹配 Publisher 和 Subscriber
std::vector<Diroute::SubscriberInfo*> Diroute::matchSubscribers(const ServiceDescription& serviceDesc) noexcept
{
    std::vector<SubscriberInfo*> matched;
    
    std::lock_guard<std::mutex> lock(m_pubSubMutex);
    
    for (auto& subscriber : m_subscribers)
    {
        // 精确匹配：service, instance, event 必须完全一致
        if (subscriber.serviceDesc == serviceDesc)
        {
            matched.push_back(&subscriber);
        }
    }
    
    return matched;
}

/// 将消息路由到订阅者的接收队列
bool Diroute::routeMessageToSubscriber(const SubscriberInfo& subscriber,
                                       uint64_t chunkOffset, uint64_t chunkSize, uint64_t payloadSize,
                                       const RuntimeName_t& publisherName) noexcept
{
    // TODO: 从共享内存中获取接收队列
    // 这里需要访问 DirouteComponents 中的接收队列
    // 暂时使用日志记录，实际实现需要：
    // 1. 获取共享内存基地址
    // 2. 根据 receiveQueueOffset 定位接收队列
    // 3. 创建 MessageHeader 并写入队列
    
    ZEROCP_LOG(Info, "Routing message to Subscriber: " << subscriber.processName.c_str()
               << " (chunkOffset: " << chunkOffset 
               << ", chunkSize: " << chunkSize
               << ", payloadSize: " << payloadSize << ")");
    
    // 创建消息头
    Popo::MessageHeader msgHeader(subscriber.serviceDesc);
    msgHeader.chunkOffset = chunkOffset;
    msgHeader.chunkSize = chunkSize;
    msgHeader.payloadSize = payloadSize;
    msgHeader.sequenceNumber = m_sequenceNumber.fetch_add(1, std::memory_order_relaxed);
    
    auto now = std::chrono::steady_clock::now();
    msgHeader.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    msgHeader.publisherName = publisherName;
    
    // TODO: 实际实现中，需要将 msgHeader 写入共享内存中的接收队列
    // 使用 LockFreeRingBuffer<MessageHeader> 的 tryPush 方法
    // 例如：
    // auto* receiveQueue = reinterpret_cast<LockFreeRingBuffer<MessageHeader, 1024>*>(
    //     static_cast<char*>(sharedMemoryBase) + subscriber.receiveQueueOffset);
    // if (!receiveQueue->tryPush(msgHeader))
    // {
    //     ZEROCP_LOG(Warn, "Subscriber receive queue is full: " << subscriber.processName.c_str());
    //     return false;
    // }
    
    ZEROCP_LOG(Info, "✓ Message routed successfully to: " << subscriber.processName.c_str()
               << " (seq: " << msgHeader.sequenceNumber << ")");
    
    return true;
}

/// 处理消息路由
/// 消息格式: "ROUTE:<publisherName>:<service>:<instance>:<event>:<chunkOffset>:<chunkSize>:<payloadSize>"
void Diroute::handleMessageRouting(const ZeroCP::Runtime::RuntimeMessage& message,
                                    ZeroCP::Runtime::IpcInterfaceCreator& creator) noexcept
{
    if (!m_memoryManager)
    {
        ZEROCP_LOG(Error, "MemoryManager not initialized");
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:MEMORY_NOT_INITIALIZED";
        creator.sendMessage(response);
        return;
    }
    
    // 解析消息
    std::istringstream iss(message);
    std::string command, publisherName, service, instance, event;
    std::string chunkOffsetStr, chunkSizeStr, payloadSizeStr;
    
    if (!std::getline(iss, command, ':') || command != "ROUTE")
    {
        ZEROCP_LOG(Warn, "Invalid ROUTE message format: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:INVALID_FORMAT";
        creator.sendMessage(response);
        return;
    }
    
    if (!std::getline(iss, publisherName, ':') ||
        !std::getline(iss, service, ':') ||
        !std::getline(iss, instance, ':') ||
        !std::getline(iss, event, ':') ||
        !std::getline(iss, chunkOffsetStr, ':') ||
        !std::getline(iss, chunkSizeStr, ':') ||
        !std::getline(iss, payloadSizeStr))
    {
        ZEROCP_LOG(Error, "Failed to parse ROUTE message: " << message);
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:PARSE_FAILED";
        creator.sendMessage(response);
        return;
    }
    
    // 转换数值
    uint64_t chunkOffset = 0, chunkSize = 0, payloadSize = 0;
    try
    {
        chunkOffset = std::stoull(chunkOffsetStr);
        chunkSize = std::stoull(chunkSizeStr);
        payloadSize = std::stoull(payloadSizeStr);
    }
    catch (const std::exception& e)
    {
        ZEROCP_LOG(Error, "Invalid numeric values in ROUTE message");
        ZeroCP::Runtime::RuntimeMessage response = "ERROR:INVALID_NUMERIC";
        creator.sendMessage(response);
        return;
    }
    
    // 创建 ServiceDescription
    ZeroCP::id_string serviceStr, instanceStr, eventStr;
    serviceStr.insert(0, service.c_str());
    instanceStr.insert(0, instance.c_str());
    eventStr.insert(0, event.c_str());
    ServiceDescription serviceDesc(serviceStr, instanceStr, eventStr);
    
    // 匹配订阅者
    auto matchedSubscribers = matchSubscribers(serviceDesc);
    
    if (matchedSubscribers.empty())
    {
        ZEROCP_LOG(Warn, "No subscribers found for: " << service << "/" << instance << "/" << event);
        ZeroCP::Runtime::RuntimeMessage response = "WARN:NO_SUBSCRIBERS";
        creator.sendMessage(response);
        return;
    }
    
    // 路由消息到所有匹配的订阅者
    RuntimeName_t pubName;
    pubName.insert(0, publisherName.c_str());
    
    bool allSuccess = true;
    for (auto* subscriber : matchedSubscribers)
    {
        if (!routeMessageToSubscriber(*subscriber, chunkOffset, chunkSize, payloadSize, pubName))
        {
            allSuccess = false;
        }
    }
    
    // 发送响应
    if (allSuccess)
    {
        std::ostringstream responseStream;
        responseStream << "OK:ROUTED:" << matchedSubscribers.size();
        ZeroCP::Runtime::RuntimeMessage response = responseStream.str();
        creator.sendMessage(response);
        ZEROCP_LOG(Info, "✓ Routed message to " << matchedSubscribers.size() << " subscriber(s)");
    }
    else
    {
        ZeroCP::Runtime::RuntimeMessage response = "WARN:PARTIAL_ROUTE";
        creator.sendMessage(response);
        ZEROCP_LOG(Warn, "⚠️  Partial routing success (some subscribers failed)");
    }
}

/// 清理已死亡进程的 Publisher/Subscriber 注册
void Diroute::cleanupDeadProcessRegistrations(uint64_t slotIndex) noexcept
{
    std::lock_guard<std::mutex> lock(m_pubSubMutex);
    
    // 查找对应的进程名称
    std::string processName;
    {
        std::lock_guard<std::mutex> procLock(m_processesMutex);
        auto it = m_registeredProcesses.find(slotIndex);
        if (it != m_registeredProcesses.end())
        {
            processName = it->second.name;
        }
        else
        {
            return; // 进程信息已不存在
        }
    }
    
    if (processName.empty())
    {
        return;
    }
    
    RuntimeName_t runtimeName;
    runtimeName.insert(0, processName.c_str());
    
    // 清理 Publisher 注册
    m_publishers.erase(
        std::remove_if(m_publishers.begin(), m_publishers.end(),
            [&runtimeName](const PublisherInfo& pub) {
                return pub.processName == runtimeName;
            }),
        m_publishers.end()
    );
    
    // 清理 Subscriber 注册
    m_subscribers.erase(
        std::remove_if(m_subscribers.begin(), m_subscribers.end(),
            [&runtimeName](const SubscriberInfo& sub) {
                return sub.processName == runtimeName;
            }),
        m_subscribers.end()
    );
    
    ZEROCP_LOG(Info, "✓ Cleaned up Publisher/Subscriber registrations for: " << processName);
}

}
}
