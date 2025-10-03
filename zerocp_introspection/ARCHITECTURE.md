# Introspection 组件架构文档

## 架构概述

Introspection 是一个独立的系统监控组件，采用经典的客户端-服务端架构设计。组件与使用它的工具完全解耦，可以被任何需要系统监控能力的应用集成。

## 设计原则

### 1. 关注点分离 (Separation of Concerns)
- **组件层 (introspection/)**: 负责数据收集和管理逻辑
- **工具层 (tools/introspection/)**: 负责用户界面和交互

### 2. 接口抽象 (Interface Abstraction)
- 明确定义的公共接口 (introspection_server.hpp, introspection_client.hpp)
- 隐藏实现细节 (使用 Impl 模式)
- 稳定的数据类型定义 (introspection_types.hpp)

### 3. 可复用性 (Reusability)
- 组件可被多个工具或应用使用
- 支持多客户端同时访问
- 独立编译为静态库

## 目录结构

```
zero_copy_framework/
│
├── introspection/                      ← 🆕 独立组件（可重用）
│   ├── CMakeLists.txt                  ← 组件构建配置
│   ├── README.md                       ← 组件文档
│   ├── ARCHITECTURE.md                 ← 架构文档（本文件）
│   ├── example_usage.cpp               ← 使用示例
│   │
│   ├── include/introspection/          ← 公共接口
│   │   ├── introspection_types.hpp     ← 数据类型定义
│   │   ├── introspection_server.hpp    ← 服务端接口
│   │   └── introspection_client.hpp    ← 客户端接口
│   │
│   └── src/                            ← 实现文件
│       ├── introspection_server.cpp    ← 服务端实现
│       └── introspection_client.cpp    ← 客户端实现
│
└── tools/introspection/                ← 🔧 监控工具（使用组件）
    ├── CMakeLists.txt                  ← 工具构建配置
    ├── README.md                       ← 工具文档
    └── introspection_main.cpp          ← TUI 工具入口
```

## 组件架构

### 层次结构

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层 (Applications)                   │
│  ┌──────────────────┐  ┌──────────────────┐                │
│  │  TUI 监控工具     │  │   其他应用       │  ...           │
│  │  (tools/)        │  │                  │                │
│  └──────────────────┘  └──────────────────┘                │
└─────────────────────────────────────────────────────────────┘
                         ↓ 使用
┌─────────────────────────────────────────────────────────────┐
│                   Introspection 组件                         │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              Client API (客户端接口)                  │   │
│  │  • connectLocal()  • getMetrics()  • subscribe()    │   │
│  └──────────────────────────────────────────────────────┘   │
│                         ↓                                    │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              Server Core (服务端核心)                 │   │
│  │  • Data Collection  • Event Publishing               │   │
│  │  • Configuration    • Multi-client Support           │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                         ↓ 访问
┌─────────────────────────────────────────────────────────────┐
│                  系统资源 (System Resources)                 │
│  /proc/meminfo  /proc/[pid]/*  /proc/net/*  /proc/loadavg  │
└─────────────────────────────────────────────────────────────┘
```

### 组件交互

```
┌──────────────┐              ┌──────────────┐
│   Client 1   │◄─────────────┤              │
├──────────────┤              │              │
│ • getMetrics │              │              │
│ • subscribe  │              │              │
└──────────────┘              │              │
                              │   Server     │
┌──────────────┐              │              │
│   Client 2   │◄─────────────┤  • collect   │
├──────────────┤              │  • notify    │
│ • getMetrics │              │  • manage    │
│ • subscribe  │              │              │
└──────────────┘              └──────────────┘
                                     ↓
┌──────────────┐              ┌──────────────┐
│   Client N   │◄─────────────┤  Monitoring  │
└──────────────┘              │    Thread    │
                              └──────────────┘
```

## 核心类设计

### 1. IntrospectionServer (服务端)

**职责**:
- 周期性收集系统数据
- 管理客户端回调
- 维护配置信息
- 发布事件通知

**关键方法**:
```cpp
class IntrospectionServer {
    // 生命周期管理
    bool start(const IntrospectionConfig& config);
    void stop();
    IntrospectionState getState() const;
    
    // 数据访问
    SystemMetrics getCurrentMetrics() const;
    SystemMetrics collectOnce();
    
    // 事件订阅
    uint32_t registerCallback(EventCallback callback);
    void unregisterCallback(uint32_t callback_id);
    
    // 配置管理
    bool updateConfig(const IntrospectionConfig& config);
    IntrospectionConfig getConfig() const;
};
```

**线程模型**:
- 主线程: 处理 API 调用
- 监控线程: 后台数据收集和事件通知

### 2. IntrospectionClient (客户端)

**职责**:
- 连接到服务端
- 提供同步查询接口
- 管理异步订阅
- 请求配置更新

**关键方法**:
```cpp
class IntrospectionClient {
    // 连接管理
    bool connectLocal(std::shared_ptr<IntrospectionServer> server);
    void disconnect();
    bool isConnected() const;
    
    // 同步查询
    bool getMetrics(SystemMetrics& metrics);
    bool getMemoryInfo(MemoryInfo& memory);
    bool getProcessList(std::vector<ProcessInfo>& processes);
    bool getConnectionList(std::vector<ConnectionInfo>& connections);
    bool getLoadInfo(LoadInfo& load);
    
    // 异步订阅
    bool subscribe(EventCallback callback);
    void unsubscribe();
    
    // 配置管理
    bool requestConfigUpdate(const IntrospectionConfig& config);
    bool requestCollectOnce(SystemMetrics& metrics);
};
```

### 3. 数据类型 (introspection_types.hpp)

**核心数据结构**:
- `MemoryInfo`: 内存信息
- `ProcessInfo`: 进程信息
- `ConnectionInfo`: 连接信息
- `LoadInfo`: 系统负载信息
- `SystemMetrics`: 聚合所有监控数据

**配置类型**:
- `IntrospectionConfig`: 监控配置选项
- `IntrospectionEvent`: 事件通知数据
- `IntrospectionState`: 服务状态枚举

## 数据流

### 数据收集流程

```
1. 监控线程唤醒
   ↓
2. collectMetrics()
   ├─ collectMemoryInfo()      → 读取 /proc/meminfo
   ├─ collectProcessInfo()     → 遍历 /proc/[pid]/
   ├─ collectConnectionInfo()  → 读取 /proc/net/tcp
   └─ collectLoadInfo()        → 读取 /proc/loadavg
   ↓
3. 应用过滤器
   ├─ applyProcessFilter()
   └─ applyConnectionFilter()
   ↓
4. 更新缓存
   └─ current_metrics_ = metrics
   ↓
5. 创建事件
   └─ IntrospectionEvent event
   ↓
6. 通知所有回调
   └─ notifyCallbacks(event)
   ↓
7. 休眠至下次周期
```

### 客户端查询流程

```
Client.getMetrics()
   ↓
Client::Impl (锁定)
   ↓
Server::getCurrentMetrics()
   ↓
Server::Impl (锁定)
   ↓
返回 current_metrics_ (拷贝)
   ↓
Client 收到数据
```

## 线程安全

### 同步机制

1. **服务端**:
   - `metrics_mutex_`: 保护 `current_metrics_`
   - `callbacks_mutex_`: 保护 `callbacks` 映射表
   - 原子变量: `state`, `should_stop`

2. **客户端**:
   - `mutex`: 保护整个客户端状态
   - 所有公共方法都加锁

### 数据拷贝策略

- 读取数据时拷贝副本，避免长时间持锁
- 事件通知时传递常量引用
- 回调函数中的异常不影响服务端

## 性能考虑

### 优化点

1. **后台收集**: 数据收集在独立线程，不阻塞查询
2. **缓存机制**: 客户端查询只读取缓存数据
3. **按需过滤**: 在数据源层面应用过滤器
4. **智能排序**: 进程按内存使用量排序，减少无效数据

### 性能指标

- 数据收集: ~100ms (取决于系统负载)
- 客户端查询: <1ms (内存拷贝)
- 事件通知延迟: 配置的 `update_interval_ms`
- 内存占用: ~200KB (静态库)

## 扩展性

### 当前支持

- ✅ 本地进程内连接
- ✅ 多客户端并发访问
- ✅ 事件驱动架构
- ✅ 可配置的监控项

### 未来扩展

- 🔜 IPC 通信支持 (共享内存、Unix socket)
- 🔜 远程网络连接
- 🔜 数据序列化/反序列化
- 🔜 历史数据记录
- 🔜 告警机制
- 🔜 更多监控指标 (磁盘、网络流量等)

## 使用场景

### 1. 系统监控工具
```
TUI 工具读取数据 → 展示可视化界面
```

### 2. 性能分析
```
应用订阅事件 → 记录性能指标 → 生成报告
```

### 3. 自动化运维
```
监控服务 → 检测异常 → 触发告警/自动恢复
```

### 4. 负载测试
```
测试程序 → 监控资源使用 → 分析瓶颈
```

## 依赖关系

### 组件依赖

```
introspection 组件:
  - C++17 标准库
  - pthread (线程支持)
  - Linux /proc 文件系统

TUI 工具:
  - introspection 组件
  - ncurses 库 (界面显示)
```

### 编译依赖

```cmake
# 组件编译
introspection (static lib)
  └─ pthread

# 工具编译
introspection_tool (executable)
  ├─ introspection (lib)
  ├─ ncurses
  └─ pthread
```

## 最佳实践

### 1. 资源管理
```cpp
// 使用智能指针管理生命周期
auto server = std::make_shared<IntrospectionServer>();
// ... server 会自动清理
```

### 2. 错误处理
```cpp
// 总是检查返回值
if (!client.getMetrics(metrics)) {
    // 处理错误
}
```

### 3. 配置调优
```cpp
// 根据需求调整更新间隔
config.update_interval_ms = 1000;  // 实时监控
config.update_interval_ms = 5000;  // 降低开销
```

### 4. 过滤器使用
```cpp
// 减少不必要的数据处理
config.process_filter = {"my_app"};  // 只监控关心的进程
```

## 总结

Introspection 组件采用清晰的分层架构和客户端-服务端模式，实现了：

- ✅ **解耦设计**: 组件与工具分离，可独立使用
- ✅ **易于集成**: 简洁的 API，丰富的示例
- ✅ **线程安全**: 完善的同步机制
- ✅ **高性能**: 后台收集，缓存机制
- ✅ **可扩展**: 支持多客户端，事件驱动
- ✅ **灵活配置**: 可定制的监控选项

这种架构使得 Introspection 不仅是一个监控工具，更是一个可重用的系统监控组件，可以被集成到任何需要运行时监控能力的应用中。

