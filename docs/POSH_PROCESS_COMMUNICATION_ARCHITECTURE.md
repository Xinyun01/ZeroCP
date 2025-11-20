# PoshRuntime 进程通信架构分析与方案建议

## 📋 目录
- [当前架构概览](#当前架构概览)
- [现有通信机制](#现有通信机制)
- [通信方案分析](#通信方案分析)
- [改进建议](#改进建议)
- [实现提示](#实现提示)

---

## 当前架构概览

### 系统组件

```
┌─────────────────────────────────────────────────────────────┐
│                    Zero Copy Framework                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────────┐         ┌──────────────────┐         │
│  │  diroute_main    │         │  PoshRuntime     │         │
│  │  (守护进程)       │◄───────►│  (客户端运行时)   │         │
│  └──────────────────┘         └──────────────────┘         │
│         │                              │                     │
│         │                              │                     │
│         ▼                              ▼                     │
│  ┌──────────────────────────────────────────────┐          │
│  │        共享内存 (zerocp_diroute_components)   │          │
│  │  - HeartbeatPool (心跳池)                    │          │
│  │  - DirouteComponents                         │          │
│  └──────────────────────────────────────────────┘          │
│                                                             │
│  ┌──────────────────────────────────────────────┐          │
│  │      Unix Domain Socket (UDS)                │          │
│  │  - udsServer.sock (守护进程服务端)            │          │
│  │  - client_<PID>.sock (客户端)                │          │
│  └──────────────────────────────────────────────┘          │
└─────────────────────────────────────────────────────────────┘
```

---

## 现有通信机制

### 1. **Unix Domain Socket (UDS) - 控制通道**

**用途：** 进程注册、心跳槽位分配、控制消息传递

**实现位置：**
- `zerocp_daemon/communication/include/runtime/ipc_interface_creator.hpp`
- `zerocp_daemon/communication/source/runtime/ipc_interface_creator.cpp`

**通信流程：**

```
客户端 (PoshRuntime)                   守护进程 (Diroute)
     │                                        │
     │ 1. createUnixDomainSocket(CLIENT)     │
     │    └─> client_<PID>.sock             │
     │                                        │
     │ 2. sendMessage("REGISTER:...")        │
     │    └─> udsServer.sock ────────────────►│
     │                                        │
     │                                        │ 3. receiveMessage()
     │                                        │    └─> 解析注册消息
     │                                        │
     │                                        │ 4. 分配心跳槽位
     │                                        │    └─> HeartbeatPool.emplace()
     │                                        │
     │ 5. receiveMessage()                    │
     │    ◄─────────────── "OK:OFFSET:<idx>"  │
     │                                        │
     │ 6. 打开共享内存                        │
     │    └─> zerocp_diroute_components      │
     │                                        │
     │ 7. 注册心跳槽位                        │
     │    └─> HeartbeatSlot.touch()          │
     │                                        │
```

**关键代码：**
```cpp
// 客户端注册
bool PoshRuntime::registerToRouteD() noexcept {
    std::ostringstream oss;
    oss << "REGISTER:" << m_runtimeName.c_str() << ":" << m_pid << ":1";
    RuntimeMessage msg = oss.str();
    return m_ipcCreator->sendMessage(msg);
}

// 守护进程处理
void Diroute::handleProcessRegistration(...) {
    // 解析 "REGISTER:<name>:<pid>:<monitored>"
    // 分配槽位
    auto slotIt = heartbeatPool.emplace();
    // 响应 "OK:OFFSET:<slotIndex>"
}
```

---

### 2. **共享内存 (Shared Memory) - 数据通道**

**用途：** 心跳时间戳、零拷贝数据传输（未来扩展）

**实现位置：**
- `zerocp_daemon/diroute/diroute_memory_manager.hpp`
- `zerocp_daemon/memory/include/heartbeat_pool.hpp`

**共享内存结构：**

```cpp
struct DirouteComponents {
    alignas(alignof(HeartbeatPool))
    std::byte m_heartbeatPoolStorage[sizeof(HeartbeatPool)];
    
    bool m_heartbeatPoolConstructed{false};
};

// HeartbeatPool 包含多个 HeartbeatSlot
// 每个槽位存储：uint64_t m_lastTimestamp (纳秒级时间戳)
```

**心跳机制：**

```
客户端线程循环 (每 100ms)          守护进程监控线程 (每 300ms)
     │                                    │
     │ 1. HeartbeatSlot.touch()          │
     │    └─> 写入当前时间戳             │
     │        (原子操作)                  │
     │                                    │
     │                                    │ 2. 遍历所有槽位
     │                                    │    └─> HeartbeatSlot.load()
     │                                    │
     │                                    │ 3. 计算时间差
     │                                    │    └─> now_ns - lastHeartbeat
     │                                    │
     │                                    │ 4. 超时检测 (>3秒)
     │                                    │    └─> 释放槽位
     │                                    │        └─> 删除注册信息
```

**关键代码：**
```cpp
// 客户端更新心跳
void PoshRuntime::heartbeatThreadFunc() noexcept {
    while (m_heartbeatRunning.load()) {
        updateHeartbeat();  // m_heartbeatSlot->touch()
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// 守护进程检测超时
void Diroute::checkHeartbeatTimeouts() noexcept {
    const uint64_t TIMEOUT_NS = 3'000'000'000ULL;  // 3秒
    for (const auto& [slotIndex, processInfo] : m_registeredProcesses) {
        uint64_t lastHeartbeat = it->load();
        uint64_t age_ns = now_ns - lastHeartbeat;
        if (age_ns > TIMEOUT_NS) {
            // 释放槽位，删除注册信息
        }
    }
}
```

---

## 通信方案分析

### ✅ 已实现的通信机制

| 机制 | 用途 | 状态 | 性能 |
|------|------|------|------|
| **UDS (控制通道)** | 进程注册、槽位分配 | ✅ 已实现 | 低延迟 (~10μs) |
| **共享内存 (心跳)** | 心跳时间戳 | ✅ 已实现 | 零拷贝 (~100ns) |
| **原子操作** | 心跳时间戳同步 | ✅ 已实现 | 无锁 |

### ⚠️ 缺失的通信机制

| 机制 | 用途 | 优先级 | 建议方案 |
|------|------|--------|----------|
| **发布-订阅 (Pub-Sub)** | 应用间数据通信 | 🔴 高 | 共享内存 + 无锁队列 |
| **请求-响应 (Req-Rep)** | 同步 RPC 调用 | 🟡 中 | UDS + 共享内存 |
| **广播/多播** | 一对多消息传递 | 🟡 中 | 共享内存 + 订阅表 |
| **零拷贝数据传输** | 大数据传输 | 🔴 高 | 共享内存池 + 相对指针 |

---

## 生产-消费进程匹配机制

### 架构概述

系统采用**路由进程（Diroute）匹配机制**，实现生产进程（Publisher）和消费进程（Subscriber）之间的零拷贝数据传输：

```
┌─────────────────────────────────────────────────────────────────┐
│                    生产-消费匹配流程                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  生产进程 (Publisher)             路由进程 (Diroute)            │
│         │                                │                       │
│         │ 1. PUBLISHER:...               │                       │
│         ├───────────────────────────────►│                       │
│         │                                │ 2. 注册 Publisher     │
│         │                                │    (service/instance/event)
│         │                                │                       │
│         │ 3. ROUTE:...:<chunkOffset>     │                       │
│         ├───────────────────────────────►│                       │
│         │                                │ 4. 匹配 Subscribers   │
│         │                                │    (精确匹配 service/instance/event)
│         │                                │                       │
│         │                                │ 5. 路由到接收队列     │
│         │                                ├───────────────────────┼──┐
│         │                                │                       │  │
│         │                                │                       │  │
│  消费进程 (Subscriber)                   │                       │  │
│         │                                │                       │  │
│         │ 6. SUBSCRIBER:...              │                       │  │
│         ├───────────────────────────────►│                       │  │
│         │                                │ 7. 注册 Subscriber    │  │
│         │                                │    (分配接收队列)     │  │
│         │                                │                       │  │
│         │                                │                       │  │
│         │ 8. 从共享内存读取接收队列       │                       │  │
│         │    (LockFreeRingBuffer)        │                       │  │
│         │◄───────────────────────────────┼───────────────────────┼──┘
│         │                                │                       │
│         │ 9. 读取 chunk 数据             │                       │
│         │    (根据 chunkOffset)          │                       │
│         │                                │                       │
└─────────┴────────────────────────────────┴───────────────────────┘
```

### 消息格式

#### 1. Publisher 注册
```
PUBLISHER:<processName>:<pid>:<service>:<instance>:<event>
```

#### 2. Subscriber 注册
```
SUBSCRIBER:<processName>:<pid>:<service>:<instance>:<event>
响应: OK:SUBSCRIBER_REGISTERED:QUEUE_OFFSET:<offset>
```

#### 3. 消息路由
```
ROUTE:<publisherName>:<service>:<instance>:<event>:<chunkOffset>:<chunkSize>:<payloadSize>
响应: OK:ROUTED:<subscriberCount>
```

### 匹配机制

**匹配规则：**
- **精确匹配**：`service`, `instance`, `event` 必须完全一致
- **多对多支持**：一个 Publisher 可以匹配多个 Subscriber
- **自动清理**：进程死亡时自动清理注册信息

**匹配流程：**
1. Publisher 发送 `ROUTE` 消息，包含 `ServiceDescription` 和 chunk 信息
2. Diroute 根据 `ServiceDescription` 匹配所有注册的 Subscriber
3. 将消息头（`MessageHeader`）写入每个匹配 Subscriber 的接收队列
4. Subscriber 从接收队列读取消息头，根据 `chunkOffset` 读取 chunk 数据

### 共享内存结构

#### MessageHeader（消息头）
```cpp
struct MessageHeader {
    id_string service;          // 服务名称
    id_string instance;         // 实例名称
    id_string event;            // 事件名称
    uint64_t chunkOffset;       // Chunk 在共享内存中的偏移量
    uint64_t chunkSize;         // Chunk 大小
    uint64_t payloadSize;       // 用户数据大小
    uint64_t sequenceNumber;    // 序列号
    uint64_t timestamp;         // 时间戳
    RuntimeName_t publisherName; // 发布者名称
};
```

#### 接收队列
- 每个 Subscriber 在共享内存中有一个独立的接收队列
- 使用 `LockFreeRingBuffer<MessageHeader, 1024>` 实现
- 队列位置由 `receiveQueueOffset` 指定

---

## 改进建议

### 1. **完善发布-订阅机制** (优先级：🔴 高) ✅ 已实现基础框架

**当前状态：**
- ✅ `Publisher` 和 `Subscriber` 注册机制已实现
- ✅ `ServiceDescription` 匹配机制已实现
- ✅ 消息路由机制已实现
- ⚠️ 共享内存接收队列待完善（TODO 标记）

**已实现功能：**

#### 方案 A：基于共享内存 + 无锁环形缓冲区 ✅

```cpp
// 共享内存中的发布-订阅结构
struct PubSubChannel {
    LockFreeRingBuffer<MessageHeader> m_messageQueue;  // 无锁队列
    std::atomic<uint64_t> m_writeIndex{0};
    std::atomic<uint64_t> m_readIndex{0};
    char m_payloadBuffer[CHANNEL_SIZE];  // 消息负载
};

// Publisher 实现
template<typename T>
class Publisher {
public:
    bool publish(const T& data) noexcept {
        // 1. 从共享内存池分配消息空间
        // 2. 序列化数据到共享内存
        // 3. 写入消息头到无锁队列
        // 4. 通知订阅者（可选：事件通知）
    }
};

// Subscriber 实现
template<typename T>
class Subscriber {
public:
    bool receive(T& data) noexcept {
        // 1. 从无锁队列读取消息头
        // 2. 从共享内存读取消息负载
        // 3. 反序列化数据
        // 4. 返回给用户
    }
};
```

**优势：**
- ✅ 零拷贝（数据在共享内存中）
- ✅ 高性能（无锁设计）
- ✅ 支持多订阅者

**已实现：**
- ✅ 使用 `LockFreeRingBuffer`（`zerocp_foundationLib/report/include/lockfree_ringbuffer.hpp`）
- ✅ 在 `Diroute` 中实现 Publisher/Subscriber 注册和匹配
- ✅ 通过 `ServiceDescription` 路由到对应的 Subscriber
- ⚠️ 接收队列在共享内存中的分配待完善（当前使用占位符）

**实现位置：**
- `zerocp_daemon/communication/include/diroute.hpp` - Diroute 类定义
- `zerocp_daemon/communication/source/diroute.cpp` - 匹配机制实现
- `zerocp_daemon/communication/include/popo/message_header.hpp` - 消息头定义

---

#### 方案 B：基于共享内存 + 订阅表

```cpp
// 共享内存中的订阅表
struct SubscriptionTable {
    struct Entry {
        RuntimeName_t subscriberName;
        uint64_t channelId;
        std::atomic<bool> isActive{true};
    };
    
    std::array<Entry, MAX_SUBSCRIBERS> m_entries;
    std::atomic<uint64_t> m_count{0};
};

// 发布时，遍历订阅表，写入所有订阅者的接收队列
```

**优势：**
- ✅ 支持多对多通信
- ✅ 动态订阅/取消订阅
- ✅ 守护进程管理订阅关系

---

### 2. **实现请求-响应机制** (优先级：🟡 中)

**场景：** 同步 RPC 调用，需要等待响应

**实现方案：**

```cpp
// 请求-响应通道
class RequestResponseChannel {
public:
    struct Request {
        uint64_t requestId;
        RuntimeName_t from;
        RuntimeName_t to;
        std::string payload;
    };
    
    struct Response {
        uint64_t requestId;
        std::string payload;
    };
    
    // 发送请求，等待响应
    std::expected<Response, Error> 
    sendRequest(const Request& req, std::chrono::milliseconds timeout) noexcept;
    
    // 处理请求，发送响应
    bool handleRequest(std::function<Response(const Request&)> handler) noexcept;
};
```

**实现提示：**
- 使用 UDS 发送请求（小消息）
- 使用共享内存传输响应数据（大消息）
- 使用条件变量或事件通知等待响应

---

### 3. **扩展零拷贝数据传输** (优先级：🔴 高)

**当前状态：**
- 共享内存池已实现（`zerocp_daemon/memory/`）
- 相对指针机制待完善

**建议实现：**

```cpp
// 在 DirouteComponents 中添加内存池
struct DirouteComponents {
    HeartbeatPool m_heartbeatPool;
    MemPoolManager m_memPoolManager;  // 新增：内存池管理器
    PubSubManager m_pubSubManager;    // 新增：发布-订阅管理器
};

// 使用相对指针实现零拷贝
template<typename T>
class SharedPtr {
    uint64_t m_offset;  // 相对于共享内存基地址的偏移
    void* m_baseAddress;
    
public:
    T* get() noexcept {
        return reinterpret_cast<T*>(
            static_cast<char*>(m_baseAddress) + m_offset
        );
    }
};
```

**实现提示：**
- 复用现有的 `MemPoolManager`（`zerocp_daemon/memory/include/mempool_manager.hpp`）
- 实现相对指针工具类（类似 iceoryx 的 `RelativePointer`）
- 在 `DirouteMemoryManager` 中初始化内存池

---

### 4. **添加事件通知机制** (优先级：🟡 中)

**场景：** 新消息到达、订阅者上线/下线、内存池状态变化

**实现方案：**

```cpp
// 事件通知通道（可选：使用 eventfd 或条件变量）
class EventNotifier {
public:
    // 通知订阅者：新消息到达
    void notifySubscribers(uint64_t channelId) noexcept;
    
    // 通知守护进程：订阅者上线
    void notifyDaemon(const RuntimeName_t& subscriber) noexcept;
};
```

**实现提示：**
- 使用 `eventfd` 实现跨进程事件通知
- 或使用共享内存中的条件变量（需要进程间同步原语）

---

## 实现提示

### 步骤 1：扩展 DirouteComponents（接收队列管理）

```cpp
// diroute_components.hpp
struct DirouteComponents {
    // 现有
    HeartbeatPool m_heartbeatPool;
    
    // 新增：接收队列池（为每个 Subscriber 分配一个队列）
    // 每个队列大小：sizeof(LockFreeRingBuffer<MessageHeader, 1024>)
    static constexpr uint64_t MAX_SUBSCRIBERS = 100;
    static constexpr uint64_t QUEUE_SIZE = 1024; // 队列容量
    
    alignas(alignof(LockFreeRingBuffer<MessageHeader, QUEUE_SIZE>))
    std::byte m_receiveQueuesStorage[
        MAX_SUBSCRIBERS * sizeof(LockFreeRingBuffer<MessageHeader, QUEUE_SIZE>)
    ];
    
    bool m_receiveQueuesConstructed{false};
    
    // 构造接收队列池
    void constructReceiveQueues() noexcept {
        if (!m_receiveQueuesConstructed) {
            for (uint64_t i = 0; i < MAX_SUBSCRIBERS; ++i) {
                auto* queuePtr = reinterpret_cast<LockFreeRingBuffer<MessageHeader, QUEUE_SIZE>*>(
                    &m_receiveQueuesStorage[i * sizeof(LockFreeRingBuffer<MessageHeader, QUEUE_SIZE>)]);
                new (queuePtr) LockFreeRingBuffer<MessageHeader, QUEUE_SIZE>();
            }
            m_receiveQueuesConstructed = true;
        }
    }
    
    // 获取接收队列（根据索引）
    LockFreeRingBuffer<MessageHeader, QUEUE_SIZE>* getReceiveQueue(uint64_t index) noexcept {
        if (index >= MAX_SUBSCRIBERS) return nullptr;
        return reinterpret_cast<LockFreeRingBuffer<MessageHeader, QUEUE_SIZE>*>(
            &m_receiveQueuesStorage[index * sizeof(LockFreeRingBuffer<MessageHeader, QUEUE_SIZE>)]);
    }
};
```

**实现提示：**
- 接收队列在 `DirouteMemoryManager::constructComponents()` 中构造
- 每个 Subscriber 注册时分配一个队列索引
- `receiveQueueOffset` 计算：`index * sizeof(LockFreeRingBuffer<MessageHeader, QUEUE_SIZE>)`

### 步骤 2：实现 Publisher/Subscriber（已实现基础框架）

**Publisher 工作流程：**
1. 注册：发送 `PUBLISHER:<name>:<pid>:<service>:<instance>:<event>` 到 Diroute
2. 发布数据：
   - 从 MemPoolManager 分配 chunk
   - 序列化数据到 chunk
   - 发送 `ROUTE:<name>:<service>:<instance>:<event>:<chunkOffset>:<chunkSize>:<payloadSize>` 到 Diroute
3. Diroute 匹配 Subscribers 并路由消息

**Subscriber 工作流程：**
1. 注册：发送 `SUBSCRIBER:<name>:<pid>:<service>:<instance>:<event>` 到 Diroute
2. 接收响应：`OK:SUBSCRIBER_REGISTERED:QUEUE_OFFSET:<offset>`
3. 打开共享内存，定位接收队列（根据 `queueOffset`）
4. 循环读取：从 `LockFreeRingBuffer` 读取 `MessageHeader`
5. 读取 chunk：根据 `chunkOffset` 从共享内存读取数据
6. 反序列化并处理数据

### 步骤 3：在 Diroute 中管理 Pub-Sub

```cpp
// diroute.hpp (扩展)
class Diroute {
private:
    // 新增：发布-订阅管理
    void handlePublisherRegistration(...) noexcept;
    void handleSubscriberRegistration(...) noexcept;
    void routeMessage(const RuntimeMessage& msg) noexcept;
    
    std::unordered_map<ServiceDescription, PubSubChannel*> m_channels;
    std::mutex m_channelsMutex;
};
```

### 步骤 4：完善 PoshRuntime 接口

```cpp
// posh_runtime.hpp (扩展)
class PoshRuntime {
public:
    // 现有接口
    bool sendMessage(const std::string& message) noexcept;
    
    // 新增：创建发布者
    template<typename T>
    Publisher<T> createPublisher(ServiceDescription& desc) noexcept {
        return Publisher<T>(desc, *this);
    }
    
    // 新增：创建订阅者
    template<typename T>
    Subscriber<T> createSubscriber(ServiceDescription& desc) noexcept {
        return Subscriber<T>(desc, *this);
    }
};
```

---

## 总结

### 当前架构优势
- ✅ 控制通道（UDS）已实现，支持进程注册
- ✅ 心跳机制完善，支持进程存活检测
- ✅ 共享内存基础设施完备

### 待完善功能
- 🔴 **发布-订阅机制**：应用间数据通信的核心
- 🔴 **零拷贝数据传输**：大数据传输的性能关键
- 🟡 **请求-响应机制**：同步 RPC 调用支持
- 🟡 **事件通知机制**：实时性提升

### 推荐实现顺序
1. ✅ **发布-订阅机制**（最高优先级）- **已实现基础框架**
2. 🔴 **零拷贝数据传输**（性能关键）- **待完善接收队列分配**
3. 🟡 **请求-响应机制**（功能完善）- **待实现**
4. 🟡 **事件通知机制**（体验优化）- **待实现**

---

## 实现状态总结

### ✅ 已实现功能

1. **Publisher/Subscriber 注册机制**
   - ✅ `handlePublisherRegistration()` - 处理 Publisher 注册
   - ✅ `handleSubscriberRegistration()` - 处理 Subscriber 注册
   - ✅ 注册信息存储在 `m_publishers` 和 `m_subscribers` 中

2. **匹配机制**
   - ✅ `matchSubscribers()` - 根据 `ServiceDescription` 精确匹配
   - ✅ 支持多对多通信（一个 Publisher 匹配多个 Subscriber）

3. **消息路由机制**
   - ✅ `handleMessageRouting()` - 处理消息路由请求
   - ✅ `routeMessageToSubscriber()` - 将消息路由到订阅者队列
   - ✅ 自动生成序列号和时间戳

4. **生命周期管理**
   - ✅ `cleanupDeadProcessRegistrations()` - 清理死亡进程的注册
   - ✅ 与心跳超时检测集成

5. **消息格式定义**
   - ✅ `MessageHeader` - 消息头结构（包含 chunk 信息）
   - ✅ `ServiceDescription` - 服务描述（支持比较操作）

### ⚠️ 待完善功能

1. **共享内存接收队列分配**
   - ⚠️ 当前使用占位符 `slotIndex * 1024`
   - 🔴 需要在 `DirouteComponents` 中实现队列池
   - 🔴 需要在 `DirouteMemoryManager` 中构造队列

2. **实际队列写入**
   - ⚠️ `routeMessageToSubscriber()` 中 TODO：实际写入队列
   - 🔴 需要获取共享内存基地址
   - 🔴 需要定位接收队列并调用 `tryPush()`

3. **Chunk 管理集成**
   - ⚠️ 需要与 `MemPoolManager` 集成
   - 🔴 Publisher 需要从内存池分配 chunk
   - 🔴 Subscriber 需要根据 `chunkOffset` 读取 chunk

### 📝 下一步工作

1. **完善接收队列分配**
   ```cpp
   // 在 DirouteComponents 中添加队列池
   // 在 DirouteMemoryManager::constructComponents() 中构造队列
   // 在 handleSubscriberRegistration() 中分配队列索引
   ```

2. **实现实际队列写入**
   ```cpp
   // 在 routeMessageToSubscriber() 中：
   // 1. 获取共享内存基地址（从 DirouteMemoryManager）
   // 2. 定位接收队列：baseAddress + subscriber.receiveQueueOffset
   // 3. 调用 receiveQueue->tryPush(msgHeader)
   ```

3. **集成 MemPoolManager**
   ```cpp
   // Publisher::publish():
   // 1. MemPoolManager::getChunk(size)
   // 2. 序列化数据到 chunk
   // 3. 获取 chunkOffset（相对地址）
   // 4. 发送 ROUTE 消息
   ```

---

## 参考实现

- **iceoryx**: [Eclipse iceoryx](https://github.com/eclipse-iceoryx/iceoryx) - 零拷贝中间件参考
- **现有代码**:
  - `zerocp_daemon/communication/include/popo/publisher.hpp` - Publisher 框架
  - `zerocp_daemon/communication/include/popo/subscriber.hpp` - Subscriber 框架
  - `zerocp_daemon/memory/include/mempool_manager.hpp` - 内存池管理
  - `zerocp_foundationLib/report/include/lockfree_ringbuffer.hpp` - 无锁队列

---

**文档版本**: v1.0  
**最后更新**: 2024  
**维护者**: Zero Copy Framework Team

