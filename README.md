# Zero Copy Framework

高性能多进程零拷贝框架，提供高效的数据传输和进程间通信解决方案。

## 项目结构

```
zero_copy_framework/
├── zerocp_core/                      # 核心框架代码
│   ├── include/                      # 核心头文件
│   └── src/                          # 核心实现
├── zerocp_daemon/                    # 守护进程实现
│   ├── include/                      # 守护进程头文件
│   ├── src/                          # 守护进程实现
│   └── daemon_main.cpp               # 守护进程入口
├── zerocp_foundationLib/             # 基础库
│   ├── concurrent/                   # 并发库（无锁队列、ABA问题解决）
│   ├── memory/                       # 内存管理
│   ├── posix/                        # POSIX 接口封装
│   └── staticstl/                    # 静态 STL 容器
├── zerocp_introspection/             # 🆕 Introspection 监控组件（独立）
│   ├── include/introspection/        # 公共头文件
│   │   ├── introspection_types.hpp   # 数据类型定义
│   │   ├── introspection_server.hpp  # 服务端接口
│   │   └── introspection_client.hpp  # 客户端接口
│   ├── src/                          # 实现文件
│   ├── test/                         # 单元测试
│   ├── ARCHITECTURE.md               # 架构文档
│   └── README.md                     # 组件文档
├── zerocp_examples/                  # 示例代码
│   ├── simple_pub_sub/               # 简单的发布-订阅示例
│   │   ├── publisher.cpp             # 发布者
│   │   └── subscriber.cpp            # 订阅者
│   └── high_perf/                    # 高性能传输示例
│       ├── sender.cpp                # 发送端
│       └── receiver.cpp              # 接收端
├── tools/                            # 开发工具
│   ├── docker/                       # Docker 配置
│   ├── introspection/                # 🔧 监控工具（TUI界面）
│   ├── scripts/                      # 辅助脚本
│   └── toolchains/                   # 工具链配置
├── build/                            # 构建输出目录
├── CMakeLists.txt                    # CMake 配置文件
├── run_tests.sh                      # 测试运行脚本
└── README.md                         # 本文件
```

## 核心功能

### 🚀 零拷贝传输
- **高效内存映射**: 利用共享内存技术实现零拷贝数据传输
- **多进程通信**: 高性能的进程间通信机制
- **内存管理**: 自动化的共享内存管理和生命周期控制

### 📚 基础库 (zerocp_foundationLib)

#### 并发库 (concurrent)
- **无锁队列**: 高性能无锁数据结构
- **ABA 问题解决**: 提供 ABA 问题的解决方案
- **线程安全**: 多线程安全的并发工具

#### 内存管理 (memory)
- 高效的内存分配器
- 内存池管理
- 零拷贝内存操作

#### POSIX 封装 (posix)
- POSIX 接口的现代 C++ 封装
- 跨平台兼容性支持

#### 静态 STL (staticstl)
- 静态分配的 STL 容器
- 无动态内存分配
- 适用于嵌入式和实时系统

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

详细文档: [Introspection 组件文档](zerocp_introspection/README.md)

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

详细文档: [监控工具文档](tools/introspection/README.md)

## 快速开始

### 系统要求

- **操作系统**: Linux (内核 2.6+)
- **编译器**: GCC 7+ 或 Clang 6+ (支持 C++17)
- **构建工具**: CMake 3.16+
- **依赖库**: 
  - pthread (线程支持)
  - ncurses (监控工具界面)

### 编译整个项目

```bash
# 克隆仓库
git clone <repository-url>
cd zero_copy_framework

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make -j$(nproc)
```

编译完成后将生成：
- **核心库**: `libzerocp_core.a`
- **基础库**: `libzerocp_foundation.a`
- **监控组件库**: `libintrospection.a`
- **守护进程**: `zerocp_daemon`
- **监控工具**: `introspection`
- **示例程序**: `simple_publisher`, `simple_subscriber`, `high_perf_sender`, `high_perf_receiver`

### 编译单独的组件

```bash
# 只编译核心库
make zerocp_core

# 只编译基础库
make zerocp_foundation

# 只编译 introspection 组件库
make introspection

# 只编译 introspection 工具
make introspection_tool

# 只编译示例
make examples
```

### 运行示例

#### 简单发布-订阅示例
```bash
# 终端 1: 启动订阅者
./build/bin/simple_subscriber

# 终端 2: 启动发布者
./build/bin/simple_publisher
```

#### 高性能传输示例
```bash
# 终端 1: 启动接收端
./build/bin/high_perf_receiver

# 终端 2: 启动发送端
./build/bin/high_perf_sender
```

### 运行监控工具

```bash
# 基本用法
./build/bin/introspection

# 显示帮助
./build/bin/introspection --help

# 自定义更新间隔（500ms）
./build/bin/introspection -i 500

# 过滤特定进程和端口
./build/bin/introspection -p nginx -c 80 -c 443
```

**快捷键**:
- `1-4`: 切换视图（概览/进程/连接/系统）
- `h`: 显示/隐藏帮助
- `q`: 退出
- `r`: 刷新

## 测试

### 编译测试

```bash
cd build

# 配置时启用测试
cmake -DBUILD_INTROSPECTION_TESTS=ON ..

# 编译测试
make introspection_tests
```

### 运行测试

```bash
# 运行所有测试
./bin/introspection_tests

# 运行特定测试套件
./bin/introspection_tests --gtest_filter=IntegrationTest.*

# 使用测试脚本
cd ..
./run_tests.sh
```

测试套件包含：
- **类型测试** - 数据结构和类型定义验证
- **服务端测试** - IntrospectionServer 功能测试
- **客户端测试** - IntrospectionClient 功能测试  
- **集成测试** - 端到端场景测试

详细信息请查看:
- [测试文档](zerocp_introspection/test/README.md)
- [快速开始指南](zerocp_introspection/test/QUICK_START.md)
- [测试总结](TESTING_SUMMARY.md)

## 文档

### 核心文档
- [项目总体架构](docs/ARCHITECTURE.md) *(待添加)*
- [API 参考](docs/API_REFERENCE.md) *(待添加)*

### 组件文档
- [Introspection 组件文档](zerocp_introspection/README.md)
- [Introspection 架构设计](zerocp_introspection/ARCHITECTURE.md)
- [Introspection 使用示例](zerocp_introspection/example_usage.cpp)
- [监控工具文档](tools/introspection/README.md)

### 基础库文档
- [并发库说明](zerocp_foundationLib/concurrent/README.md)

### 示例代码
- [简单发布-订阅](zerocp_examples/simple_pub_sub/)
- [高性能传输](zerocp_examples/high_perf/)

## 开发指南

### 添加新的示例

```bash
# 创建新的示例目录
mkdir zerocp_examples/my_example
cd zerocp_examples/my_example

# 创建 CMakeLists.txt 和源文件
touch CMakeLists.txt my_example.cpp

# 在父 CMakeLists.txt 中添加子目录
```

### 集成到其他项目

#### 使用 CMake

```cmake
# 添加 zero_copy_framework 子目录
add_subdirectory(path/to/zero_copy_framework)

# 链接核心库
target_link_libraries(your_target
    PRIVATE
        zerocp_core
        zerocp_foundation
)

# 如果需要监控功能
target_link_libraries(your_target
    PRIVATE
        introspection
        pthread
)
```

#### 直接链接静态库

```bash
g++ your_app.cpp \
    -I/path/to/zero_copy_framework/zerocp_core/include \
    -I/path/to/zero_copy_framework/zerocp_introspection/include \
    -L/path/to/build/lib \
    -lzerocp_core -lzerocp_foundation -lintrospection \
    -lpthread -o your_app
```

## 性能特点

- **零拷贝传输**: 消除数据拷贝开销，直接共享内存访问
- **低延迟**: 优化的进程间通信，微秒级延迟
- **高吞吐**: 支持 GB/s 级别的数据传输速率
- **无锁设计**: 并发库使用无锁算法，避免锁竞争
- **内存高效**: 静态内存分配，无碎片问题

## 性能测试

*(待添加性能基准测试结果)*

## 贡献

欢迎贡献代码和功能改进！

### 贡献流程
1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

### 代码规范
- 遵循 C++17 标准
- 使用一致的代码风格（参考现有代码）
- 添加必要的注释和文档
- 为新功能添加测试

## 许可证

MIT License

## 联系方式

*(待添加项目维护者联系方式)*

## 致谢

感谢所有贡献者对本项目的支持！
