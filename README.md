# Zero Copy Framework

高性能多进程零拷贝框架，提供高效的数据传输和进程间通信解决方案。

## 项目结构

```
zero_copy_framework/
├── core/                         # 核心框架代码
├── daemon/                       # 守护进程实现
├── introspection/                # 🆕 Introspection 监控组件（独立）
│   ├── include/introspection/    # 公共头文件
│   │   ├── introspection_types.hpp    # 数据类型定义
│   │   ├── introspection_server.hpp   # 服务端接口
│   │   └── introspection_client.hpp   # 客户端接口
│   └── src/                      # 实现文件
├── platform/                     # 平台相关代码
├── tools/                        # 开发工具
│   └── introspection/            # 🔧 监控工具（使用组件）
│       └── introspection_main.cpp     # TUI工具入口
└── README.md
```

## 核心功能

### 🚀 零拷贝传输
- 高效的内存映射技术
- 零拷贝数据传输
- 多进程共享内存管理

### 🔍 Introspection 监控组件

**独立的系统监控组件** - 采用客户端-服务端架构，提供运行时数据收集和查询能力

#### 组件特性
- **客户端-服务端架构**: 服务端负责数据收集，支持多客户端访问
- **双模式访问**: 同步查询和异步订阅两种模式
- **完整监控**: 内存、进程、连接、系统负载全方位监控
- **灵活配置**: 可配置更新间隔和过滤器
- **事件驱动**: 发布-订阅模式，支持事件回调

#### 组件使用示例

```cpp
// 创建并启动服务端
auto server = std::make_shared<IntrospectionServer>();
IntrospectionConfig config;
config.update_interval_ms = 1000;
server->start(config);

// 创建客户端并连接
IntrospectionClient client;
client.connectLocal(server);

// 同步查询数据
SystemMetrics metrics;
client.getMetrics(metrics);

// 异步订阅事件
client.subscribe([](const IntrospectionEvent& event) {
    // 处理事件
});
```

详细文档: [Introspection 组件文档](introspection/README.md)

### 🔧 Introspection 监控工具

**TUI 可视化工具** - 基于 Introspection 组件的实时监控界面

- **实时显示**: 内存、进程、连接状态实时可视化
- **多视图切换**: 概览、进程、连接、系统四种视图
- **交互操作**: 支持键盘快捷键，提供帮助信息
- **过滤功能**: 支持进程名称和端口过滤

```bash
# 启动监控工具
./build/bin/introspection

# 监控特定进程
./build/bin/introspection --process nginx --process redis

# 监控特定端口
./build/bin/introspection --connection 80 --connection 443
```

## 快速开始

### 编译整个项目
```bash
mkdir build && cd build
cmake ..
make
```

这将编译：
- `libintrospection.a` - Introspection 监控组件库
- `introspection` - Introspection TUI 监控工具

### 编译单独的组件

```bash
# 只编译 introspection 组件库
make introspection

# 只编译 introspection 工具
make introspection_tool
```

### 运行监控工具
```bash
# 基本用法
./build/bin/introspection

# 显示帮助
./build/bin/introspection --help

# 自定义更新间隔
./build/bin/introspection -i 500

# 过滤特定进程和端口
./build/bin/introspection -p nginx -c 80 -c 443
```

## 系统要求

- Linux 系统
- CMake 3.16+
- C++17 编译器
- ncurses 库 (用于监控工具)

## 测试

### 运行测试

```bash
# 编译测试
cd build
cmake -DBUILD_INTROSPECTION_TESTS=ON ..
make introspection_tests

# 运行所有测试
./bin/introspection_tests

# 运行特定测试
./bin/introspection_tests --gtest_filter=IntegrationTest.*
```

测试套件包含：
- **类型测试** - 数据结构和类型定义
- **服务端测试** - IntrospectionServer 功能
- **客户端测试** - IntrospectionClient 功能  
- **集成测试** - 端到端场景测试

详细信息请查看 [`introspection/test/README.md`](introspection/test/README.md) 和 [`introspection/test/QUICK_START.md`](introspection/test/QUICK_START.md)

## 文档

- [组件架构设计](introspection/ARCHITECTURE.md) - 详细的架构文档
- [使用示例](introspection/example_usage.cpp) - 完整的代码示例
- [测试指南](introspection/test/README.md) - 测试说明文档

## 贡献

欢迎贡献代码和功能改进！

## 许可证

MIT License
