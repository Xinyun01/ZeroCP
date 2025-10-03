# Introspection 组件

## 概述

Introspection 是 Zero Copy Framework 的独立监控组件，提供系统运行时数据收集和查询能力。采用客户端-服务端架构，支持多个客户端同时访问监控数据。

## 架构设计

```
introspection/                          ← 独立组件
├── CMakeLists.txt                      ← 构建配置
├── include/introspection/              ← 公共头文件
│   ├── introspection_types.hpp         ← 数据类型定义
│   ├── introspection_server.hpp        ← 服务端接口
│   └── introspection_client.hpp        ← 客户端接口
└── source/                             ← 实现文件
    ├── introspection_server.cpp        ← 服务端实现
    └── introspection_client.cpp        ← 客户端实现
```

## 核心特性

### 🔌 客户端-服务端架构
- **服务端**: 负责数据收集和维护，支持多客户端连接
- **客户端**: 提供同步查询和异步订阅两种访问模式
- **解耦设计**: 组件可独立使用，不依赖 TUI 界面

### 📊 监控能力
- **内存监控**: 总内存、已使用、空闲、共享、缓冲区、缓存等
- **进程监控**: PID、名称、内存使用、CPU使用率、状态、命令行
- **连接监控**: 本地/远程地址、协议、状态、数据传输量
- **系统负载**: CPU使用率、负载平均值、核心数

### ⚙️ 灵活配置
- 可配置的更新间隔
- 进程名称过滤
- 连接端口过滤
- 可选择性启用/禁用各类监控

### 🔔 事件驱动
- 支持事件回调机制
- 发布-订阅模式
- 异步事件通知

## 快速开始

### 编译组件

```bash
cd /path/to/zero_copy_framework
mkdir build && cd build
cmake ..
make introspection
```

这将生成静态库 `libintrospection.a`。

### 基本用法

#### 1. 创建服务端并启动监控

```cpp
#include "introspection/introspection_server.hpp"

using namespace zero_copy::introspection;

// 创建服务端
auto server = std::make_shared<IntrospectionServer>();

// 配置监控选项
IntrospectionConfig config;
config.update_interval_ms = 1000;  // 1秒更新间隔
config.process_filter = {"nginx", "redis"};  // 只监控这些进程
config.connection_filter = {80, 443};  // 只监控这些端口

// 启动服务
if (server->start(config)) {
    std::cout << "监控服务已启动" << std::endl;
}
```

#### 2. 创建客户端并连接

```cpp
#include "introspection/introspection_client.hpp"

using namespace zero_copy::introspection;

// 创建客户端
IntrospectionClient client;

// 连接到本地服务端
if (client.connectLocal(server)) {
    std::cout << "已连接到监控服务" << std::endl;
}
```

#### 3. 同步查询数据

```cpp
// 获取完整的系统指标
SystemMetrics metrics;
if (client.getMetrics(metrics)) {
    std::cout << "内存使用率: " << metrics.memory.memory_usage_percent << "%" << std::endl;
    std::cout << "进程数量: " << metrics.processes.size() << std::endl;
    std::cout << "连接数量: " << metrics.connections.size() << std::endl;
}

// 或者只获取特定信息
MemoryInfo memory;
if (client.getMemoryInfo(memory)) {
    std::cout << "总内存: " << memory.total_memory << " bytes" << std::endl;
}

std::vector<ProcessInfo> processes;
if (client.getProcessList(processes)) {
    for (const auto& proc : processes) {
        std::cout << "进程: " << proc.name << " (PID: " << proc.pid << ")" << std::endl;
    }
}
```

#### 4. 异步订阅事件

```cpp
// 订阅监控事件
client.subscribe([](const IntrospectionEvent& event) {
    if (event.type == IntrospectionEventType::SYSTEM_UPDATE) {
        std::cout << "收到系统更新事件" << std::endl;
        std::cout << "内存使用率: " 
                  << event.metrics.memory.memory_usage_percent << "%" << std::endl;
    } else if (event.type == IntrospectionEventType::ERROR) {
        std::cerr << "错误: " << event.error_message << std::endl;
    }
});

// 取消订阅
client.unsubscribe();
```

#### 5. 动态更新配置

```cpp
// 更新监控配置
IntrospectionConfig new_config;
new_config.update_interval_ms = 500;  // 改为500ms更新间隔
new_config.process_filter = {"postgres"};  // 改为监控postgres

if (client.requestConfigUpdate(new_config)) {
    std::cout << "配置已更新" << std::endl;
}
```

#### 6. 请求立即收集数据

```cpp
// 请求立即收集一次数据（不等待定时更新）
SystemMetrics metrics;
if (client.requestCollectOnce(metrics)) {
    std::cout << "立即收集完成" << std::endl;
}
```

#### 7. 清理资源

```cpp
// 断开客户端连接
client.disconnect();

// 停止服务端
server->stop();
```

## 完整示例

```cpp
#include "introspection/introspection_server.hpp"
#include "introspection/introspection_client.hpp"
#include <iostream>
#include <thread>

using namespace zero_copy::introspection;

int main() {
    // 1. 创建并配置服务端
    auto server = std::make_shared<IntrospectionServer>();
    IntrospectionConfig config;
    config.update_interval_ms = 1000;
    
    if (!server->start(config)) {
        std::cerr << "无法启动监控服务" << std::endl;
        return 1;
    }

    // 2. 创建并连接客户端
    IntrospectionClient client;
    if (!client.connectLocal(server)) {
        std::cerr << "无法连接到监控服务" << std::endl;
        return 1;
    }

    // 3. 订阅异步事件
    client.subscribe([](const IntrospectionEvent& event) {
        if (event.type == IntrospectionEventType::SYSTEM_UPDATE) {
            std::cout << "内存: " << event.metrics.memory.memory_usage_percent << "% | "
                      << "进程: " << event.metrics.processes.size() << " | "
                      << "连接: " << event.metrics.connections.size() << std::endl;
        }
    });

    // 4. 运行一段时间
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // 5. 同步查询数据
    SystemMetrics metrics;
    if (client.getMetrics(metrics)) {
        std::cout << "\n最终统计:" << std::endl;
        std::cout << "内存使用率: " << metrics.memory.memory_usage_percent << "%" << std::endl;
        std::cout << "CPU使用率: " << metrics.load.cpu_usage_percent << "%" << std::endl;
    }

    // 6. 清理
    client.disconnect();
    server->stop();

    return 0;
}
```

## API 参考

### IntrospectionServer

主要方法：
- `bool start(const IntrospectionConfig& config)` - 启动监控服务
- `void stop()` - 停止监控服务
- `IntrospectionState getState() const` - 获取当前状态
- `SystemMetrics getCurrentMetrics() const` - 获取当前系统指标
- `uint32_t registerCallback(EventCallback callback)` - 注册事件回调
- `void unregisterCallback(uint32_t callback_id)` - 取消注册回调
- `bool updateConfig(const IntrospectionConfig& config)` - 更新配置
- `SystemMetrics collectOnce()` - 立即收集一次数据

### IntrospectionClient

主要方法：
- `bool connectLocal(std::shared_ptr<IntrospectionServer> server)` - 连接到本地服务端
- `void disconnect()` - 断开连接
- `bool isConnected() const` - 检查连接状态
- `bool getMetrics(SystemMetrics& metrics)` - 获取完整系统指标
- `bool getMemoryInfo(MemoryInfo& memory)` - 获取内存信息
- `bool getProcessList(std::vector<ProcessInfo>& processes)` - 获取进程列表
- `bool getConnectionList(std::vector<ConnectionInfo>& connections)` - 获取连接列表
- `bool getLoadInfo(LoadInfo& load)` - 获取系统负载信息
- `bool subscribe(EventCallback callback)` - 订阅事件
- `void unsubscribe()` - 取消订阅
- `bool requestConfigUpdate(const IntrospectionConfig& config)` - 请求更新配置
- `bool requestCollectOnce(SystemMetrics& metrics)` - 请求立即收集数据

### 数据类型

详见 `introspection_types.hpp`：
- `MemoryInfo` - 内存信息
- `ProcessInfo` - 进程信息
- `ConnectionInfo` - 连接信息
- `LoadInfo` - 系统负载信息
- `SystemMetrics` - 完整系统指标
- `IntrospectionConfig` - 监控配置
- `IntrospectionEvent` - 监控事件
- `IntrospectionEventType` - 事件类型
- `IntrospectionState` - 服务状态

## 集成到其他项目

### CMake 集成

```cmake
# 添加 introspection 组件
add_subdirectory(path/to/introspection)

# 链接到你的目标
target_link_libraries(your_target
    PRIVATE
        introspection
        pthread
)

# 包含头文件
target_include_directories(your_target
    PRIVATE
        path/to/introspection/include
)
```

### 作为静态库使用

```bash
# 编译生成静态库
cd introspection
mkdir build && cd build
cmake ..
make

# 在你的项目中链接
g++ your_app.cpp -I/path/to/introspection/include \
    -L/path/to/build/lib -lintrospection -lpthread -o your_app
```

## 注意事项

1. **线程安全**: 服务端和客户端都是线程安全的
2. **资源管理**: 使用智能指针管理生命周期
3. **错误处理**: 所有API都返回状态码，务必检查返回值
4. **Linux专用**: 当前实现依赖 `/proc` 文件系统，仅支持Linux
5. **权限要求**: 某些系统信息可能需要适当权限才能访问

## 性能考虑

- 数据收集在独立线程中进行，不阻塞调用者
- 客户端查询只是读取缓存的数据，开销很小
- 可通过配置更新间隔来平衡实时性和性能
- 使用过滤器可以减少不必要的数据处理

## 扩展开发

### 添加新的监控数据

1. 在 `introspection_types.hpp` 中添加新的数据结构
2. 在 `IntrospectionServer` 中实现数据收集逻辑
3. 在 `IntrospectionClient` 中添加查询接口

### 支持远程连接

当前实现仅支持本地连接（进程内），未来可扩展：
- 添加 IPC 通信机制（共享内存、Unix socket等）
- 添加网络通信支持
- 实现序列化/反序列化

## 许可证

MIT License

