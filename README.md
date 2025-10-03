# Zero Copy Framework

高性能多进程零拷贝通信框架，基于共享内存技术实现进程间的高效数据传输和通信。

## 📋 目录

- [项目简介](#项目简介)
- [项目结构](#项目结构)
- [核心功能](#核心功能)
- [快速开始](#快速开始)
- [详细使用](#详细使用)
- [测试](#测试)
- [性能特点](#性能特点)
- [开发指南](#开发指南)
- [文档](#文档)
- [贡献](#贡献)
- [许可证](#许可证)

## 项目简介

Zero Copy Framework 是一个专为高性能场景设计的进程间通信框架，采用零拷贝技术消除数据传输过程中的内存拷贝开销，实现微秒级延迟和 GB/s 级吞吐量。

### 核心特性

- ✨ **零拷贝传输** - 基于共享内存，消除数据拷贝开销
- 🚀 **高性能** - 微秒级延迟，GB/s 级吞吐量
- 🔒 **无锁设计** - 并发库采用无锁算法，避免锁竞争
- 📊 **实时监控** - 完整的运行时监控和可视化工具
- 🛠️ **模块化设计** - 组件解耦，可独立使用
- 💪 **类型安全** - 现代 C++17，强类型设计

### 适用场景

- 高频交易系统
- 实时数据处理
- 游戏服务器
- 音视频流媒体
- 机器人控制系统
- 任何需要高性能 IPC 的场景

## 项目结构

```
zero_copy_framework/
│
├── zerocp_core/                      # 核心框架代码
│   ├── include/                      # 公共头文件
│   │   ├── ipc_channel.hpp           # IPC 通道接口
│   │   ├── memory_pool.hpp           # 内存池管理
│   │   └── shared_memory.hpp         # 共享内存封装
│   └── source/                       # 核心实现
│       ├── ipc_channel.cpp
│       ├── memory_pool.cpp
│       └── shared_memory.cpp
│
├── zerocp_daemon/                    # 守护进程实现
│   ├── include/                      # 守护进程头文件
│   │   └── resource_manager.hpp      # 资源管理器
│   ├── source/                       # 守护进程实现
│   │   └── resource_manager.cpp
│   └── daemon_main.cpp               # 守护进程入口
│
├── zerocp_foundationLib/             # 基础库集合
│   ├── concurrent/                   # 并发库（无锁队列、ABA 问题解决）
│   │   ├── include/                  # 并发库头文件
│   │   ├── source/                   # 并发库实现
│   │   └── README.md                 # 并发库文档
│   ├── memory/                       # 内存管理（分配器、内存池）
│   ├── posix/                        # POSIX 接口封装
│   │   └── ipc/                      # IPC 相关封装
│   │       └── source/               # IPC 实现
│   └── staticstl/                    # 静态 STL 容器
│       ├── include/                  # 静态容器头文件
│       └── detail/                   # 实现细节
│
├── zerocp_introspection/             # 监控组件（独立）
│   ├── include/introspection/        # 公共头文件
│   │   ├── introspection_types.hpp   # 数据类型定义
│   │   ├── introspection_server.hpp  # 服务端接口
│   │   └── introspection_client.hpp  # 客户端接口
│   ├── source/                       # 实现文件
│   │   ├── introspection_server.cpp  # 服务端实现
│   │   └── introspection_client.cpp  # 客户端实现
│   ├── test/                         # 单元测试
│   │   ├── test_introspection_types.cpp
│   │   ├── test_introspection_server.cpp
│   │   ├── test_introspection_client.cpp
│   │   ├── test_integration.cpp
│   │   ├── README.md                 # 测试文档
│   │   └── QUICK_START.md            # 测试快速开始
│   ├── example_usage.cpp             # 使用示例
│   ├── introspection-config.cmake.in # CMake 配置模板
│   ├── ARCHITECTURE.md               # 架构文档
│   └── README.md                     # 组件文档
│
├── zerocp_examples/                  # 示例代码
│   ├── simple_pub_sub/               # 简单发布-订阅示例
│   │   ├── publisher.cpp             # 发布者
│   │   ├── subscriber.cpp            # 订阅者
│   │   └── CMakeLists.txt
│   └── high_perf/                    # 高性能传输示例
│       ├── sender.cpp                # 发送端
│       ├── receiver.cpp              # 接收端
│       └── CMakeLists.txt
│
├── tools/                            # 开发工具
│   ├── docker/                       # Docker 配置
│   ├── introspection/                # 监控工具（TUI 界面）
│   │   ├── introspection_main.cpp    # 工具入口
│   │   ├── README.md                 # 工具文档
│   │   └── CMakeLists.txt
│   ├── scripts/                      # 辅助脚本
│   │   ├── clang_format.sh           # 代码格式化
│   │   ├── clang_tidy_check.sh       # 静态检查
│   │   ├── lcov_generate.sh          # 代码覆盖率
│   │   └── ...                       # 其他工具脚本
│   └── toolchains/                   # 工具链配置
│       └── qnx/                      # QNX 工具链
│
├── build/                            # 构建输出目录（自动生成）
├── CMakeLists.txt                    # 根 CMake 配置文件
└── README.md                         # 本文件
```

## 核心功能

### 🚀 零拷贝传输 (zerocp_core)

核心通信框架，提供高效的进程间数据传输能力。

**主要组件：**
- **IPC Channel** (`ipc_channel.hpp`) - 高性能进程间通信通道
- **Shared Memory** (`shared_memory.hpp`) - 共享内存管理和映射
- **Memory Pool** (`memory_pool.hpp`) - 内存池分配和管理

**特性：**
- 零拷贝数据传输
- 自动化内存生命周期管理
- 支持多进程并发访问
- 高效的内存映射机制

### 📚 基础库 (zerocp_foundationLib)

提供底层基础设施和通用组件。

#### 并发库 (concurrent)
- **无锁队列** - 高性能 Lock-free 数据结构
- **ABA 问题解决** - 提供 ABA 问题的完整解决方案
- **线程安全** - 多线程安全的并发工具
- 详细文档：[concurrent/README.md](zerocp_foundationLib/concurrent/README.md)

#### 内存管理 (memory)
- 高效的内存分配器
- 内存池管理
- 零拷贝内存操作
- 内存对齐和优化

#### POSIX 封装 (posix)
- POSIX 接口的现代 C++ 封装
- IPC 机制封装（共享内存、信号量等）
- 跨平台兼容性支持
- 异常安全的资源管理

#### 静态 STL (staticstl)
- 静态分配的 STL 兼容容器
- 无动态内存分配
- 编译期大小确定
- 适用于嵌入式和实时系统

### 🔍 Introspection 监控组件

**独立的系统监控组件** - 采用客户端-服务端架构，提供运行时数据收集和查询能力。

#### 组件特性
- ✅ **客户端-服务端架构** - 服务端负责数据收集，支持多客户端访问
- ✅ **双模式访问** - 同步查询和异步订阅两种模式
- ✅ **完整监控** - 内存、进程、连接、系统负载全方位监控
- ✅ **灵活配置** - 可配置更新间隔和过滤器
- ✅ **事件驱动** - 发布-订阅模式，支持事件回调
- ✅ **独立部署** - 可作为独立库集成到其他项目

#### 核心 API

**服务端 API**
```cpp
#include <introspection/introspection_server.hpp>

// 创建并启动服务端
auto server = std::make_shared<IntrospectionServer>();
IntrospectionConfig config;
config.update_interval_ms = 1000;  // 1秒更新间隔
server->start(config);

// 停止服务端
server->stop();
```

**客户端 API**
```cpp
#include <introspection/introspection_client.hpp>

// 创建客户端并连接
IntrospectionClient client;
client.connectLocal(server);  // 本地连接
// 或 client.connectRemote("127.0.0.1", 8080);  // 远程连接

// 同步查询数据
SystemMetrics metrics;
if (client.getMetrics(metrics)) {
    std::cout << "CPU使用率: " << metrics.cpu_usage << "%\n";
    std::cout << "内存使用: " << metrics.memory_usage_mb << " MB\n";
}

// 异步订阅事件
client.subscribe([](const IntrospectionEvent& event) {
    if (event.type == EventType::MEMORY_USAGE_HIGH) {
        std::cout << "警告：内存使用率过高！\n";
    }
});

// 断开连接
client.disconnect();
```

**数据类型**
```cpp
#include <introspection/introspection_types.hpp>

// 系统指标
struct SystemMetrics {
    double cpu_usage;           // CPU使用率 (%)
    uint64_t memory_usage_mb;   // 内存使用 (MB)
    uint64_t memory_total_mb;   // 总内存 (MB)
    uint32_t process_count;     // 进程数量
    uint32_t connection_count;  // 连接数量
    double load_average[3];     // 系统负载
};

// 事件类型
enum class EventType {
    MEMORY_USAGE_HIGH,
    CPU_USAGE_HIGH,
    PROCESS_CREATED,
    PROCESS_TERMINATED,
    CONNECTION_ESTABLISHED,
    CONNECTION_CLOSED
};
```

**详细文档：** [zerocp_introspection/README.md](zerocp_introspection/README.md)  
**架构设计：** [zerocp_introspection/ARCHITECTURE.md](zerocp_introspection/ARCHITECTURE.md)  
**完整示例：** [zerocp_introspection/example_usage.cpp](zerocp_introspection/example_usage.cpp)

### 🔧 Introspection 监控工具

**TUI 可视化工具** - 基于 Introspection 组件的实时监控界面。

#### 工具特性
- 📊 **实时显示** - 内存、进程、连接状态实时可视化
- 🖥️ **多视图切换** - 概览、进程、连接、系统四种视图
- ⌨️ **交互操作** - 支持键盘快捷键，提供帮助信息
- 🔍 **过滤功能** - 支持进程名称和端口过滤
- 🎨 **美观界面** - 基于 ncurses 的现代 TUI 界面

#### 使用方法

```bash
# 基本用法
./build/bin/introspection

# 显示帮助
./build/bin/introspection --help

# 自定义更新间隔（500ms）
./build/bin/introspection -i 500

# 监控特定进程
./build/bin/introspection --process nginx --process redis

# 监控特定端口
./build/bin/introspection --connection 80 --connection 443

# 组合使用
./build/bin/introspection -i 1000 -p nginx -c 80 -c 443
```

#### 快捷键

| 快捷键 | 功能 |
|--------|------|
| `1` | 切换到概览视图 |
| `2` | 切换到进程视图 |
| `3` | 切换到连接视图 |
| `4` | 切换到系统视图 |
| `h` | 显示/隐藏帮助 |
| `r` | 手动刷新 |
| `q` | 退出程序 |

**详细文档：** [tools/introspection/README.md](tools/introspection/README.md)

### 🎯 守护进程 (zerocp_daemon)

系统守护进程，负责资源管理和协调。

**主要功能：**
- 资源生命周期管理
- 进程间资源协调
- 自动清理和回收
- 系统监控和日志

## 快速开始

### 系统要求

| 组件 | 要求 |
|------|------|
| **操作系统** | Linux (内核 2.6+) |
| **编译器** | GCC 7+ 或 Clang 6+ (支持 C++17) |
| **构建工具** | CMake 3.16+ |
| **依赖库** | pthread (线程支持)<br>ncurses (监控工具界面，可选) |

### 编译整个项目

```bash
# 1. 克隆仓库
git clone <repository-url>
cd zero_copy_framework

# 2. 创建构建目录
mkdir build && cd build

# 3. 配置项目
cmake ..

# 4. 编译（使用所有可用 CPU 核心）
make -j$(nproc)
```

#### 编译输出

编译完成后将在 `build/` 目录下生成：

**库文件** (`build/lib/`)
- `libzerocp_core.a` - 核心通信库
- `libzerocp_foundation.a` - 基础库集合
- `libintrospection.a` - 监控组件库

**可执行文件** (`build/bin/`)
- `zerocp_daemon` - 系统守护进程
- `introspection` - 监控工具（TUI）
- `simple_publisher` - 发布者示例
- `simple_subscriber` - 订阅者示例
- `high_perf_sender` - 高性能发送端示例
- `high_perf_receiver` - 高性能接收端示例
- `introspection_tests` - 单元测试（如启用测试）

### 编译单独的组件

```bash
# 进入构建目录
cd build

# 只编译核心库
make zerocp_core

# 只编译基础库
make zerocp_foundation

# 只编译 introspection 组件库
make introspection

# 只编译 introspection 工具
make introspection_tool

# 只编译守护进程
make zerocp_daemon

# 只编译示例
make examples
```

### 运行示例

#### 简单发布-订阅示例

演示基本的发布-订阅模式通信。

```bash
# 终端 1: 启动订阅者
./build/bin/simple_subscriber

# 终端 2: 启动发布者
./build/bin/simple_publisher
```

#### 高性能传输示例

演示高吞吐量的数据传输。

```bash
# 终端 1: 启动接收端
./build/bin/high_perf_receiver

# 终端 2: 启动发送端
./build/bin/high_perf_sender
```

### 运行监控工具

```bash
# 基本用法（使用默认配置）
./build/bin/introspection

# 自定义更新间隔（500ms）
./build/bin/introspection -i 500

# 过滤特定进程和端口
./build/bin/introspection -p nginx -c 80 -c 443

# 查看完整帮助
./build/bin/introspection --help
```

## 详细使用

### 集成到其他项目

#### 方式一：使用 CMake 子项目

```cmake
# 在你的 CMakeLists.txt 中

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

# 添加头文件路径（如果需要）
target_include_directories(your_target
    PRIVATE
        path/to/zero_copy_framework/zerocp_core/include
        path/to/zero_copy_framework/zerocp_introspection/include
)
```

#### 方式二：直接链接静态库

```bash
# 编译命令
g++ your_app.cpp \
    -std=c++17 \
    -I/path/to/zero_copy_framework/zerocp_core/include \
    -I/path/to/zero_copy_framework/zerocp_introspection/include \
    -L/path/to/build/lib \
    -lzerocp_core -lzerocp_foundation -lintrospection \
    -lpthread \
    -o your_app
```

#### 方式三：使用 find_package（如果已安装）

```cmake
# 查找 introspection 包
find_package(introspection REQUIRED)

# 链接库
target_link_libraries(your_target
    PRIVATE
        introspection::introspection
)
```

### 基本使用示例

#### 零拷贝数据传输

```cpp
#include <ipc_channel.hpp>
#include <shared_memory.hpp>

// 创建共享内存
auto shm = std::make_shared<SharedMemory>("my_shm", 1024 * 1024);

// 创建 IPC 通道
IPCChannel channel("my_channel", shm);

// 发送数据（零拷贝）
std::vector<uint8_t> data(1024);
channel.send(data.data(), data.size());

// 接收数据（零拷贝）
void* recv_ptr = channel.receive();
```

#### 使用监控组件

完整示例请查看 [zerocp_introspection/example_usage.cpp](zerocp_introspection/example_usage.cpp)

```cpp
#include <introspection/introspection_server.hpp>
#include <introspection/introspection_client.hpp>

int main() {
    // 创建并启动服务端
    auto server = std::make_shared<IntrospectionServer>();
    IntrospectionConfig config;
    config.update_interval_ms = 1000;
    server->start(config);
    
    // 创建客户端并连接
    IntrospectionClient client;
    client.connectLocal(server);
    
    // 查询系统指标
    SystemMetrics metrics;
    if (client.getMetrics(metrics)) {
        std::cout << "CPU: " << metrics.cpu_usage << "%\n";
        std::cout << "Memory: " << metrics.memory_usage_mb << " MB\n";
    }
    
    // 订阅事件
    client.subscribe([](const IntrospectionEvent& event) {
        std::cout << "Event: " << static_cast<int>(event.type) << "\n";
    });
    
    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // 清理
    client.disconnect();
    server->stop();
    
    return 0;
}
```

## 测试

### 配置测试

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

# 显示详细输出
./bin/introspection_tests --gtest_verbose

# 使用测试脚本（从项目根目录）
cd ..
./run_tests.sh
```

### 测试套件

| 测试套件 | 文件 | 说明 |
|---------|------|------|
| **类型测试** | `test_introspection_types.cpp` | 数据结构和类型定义验证 |
| **服务端测试** | `test_introspection_server.cpp` | IntrospectionServer 功能测试 |
| **客户端测试** | `test_introspection_client.cpp` | IntrospectionClient 功能测试 |
| **集成测试** | `test_integration.cpp` | 端到端场景测试 |

### 测试文档

- [测试文档](zerocp_introspection/test/README.md) - 详细的测试说明
- [快速开始](zerocp_introspection/test/QUICK_START.md) - 测试快速入门
- [测试总结](TESTING_SUMMARY.md) - 测试结果总结（如果存在）

### 代码覆盖率

```bash
# 生成代码覆盖率报告
cd tools/scripts
./lcov_generate.sh
```

## 性能特点

### 核心性能指标

| 指标 | 数值 | 说明 |
|------|------|------|
| **延迟** | < 1μs | 进程间单次消息传输延迟 |
| **吞吐量** | > 10 GB/s | 大块数据传输吞吐量 |
| **并发连接** | 1000+ | 支持的并发连接数 |
| **内存开销** | 最小化 | 静态分配，无碎片 |

### 性能优化

- ✅ **零拷贝传输** - 消除数据拷贝开销，直接共享内存访问
- ✅ **低延迟** - 优化的进程间通信，微秒级延迟
- ✅ **高吞吐** - 支持 GB/s 级别的数据传输速率
- ✅ **无锁设计** - 并发库使用无锁算法，避免锁竞争
- ✅ **内存高效** - 静态内存分配，无碎片问题
- ✅ **CPU 亲和性** - 支持 CPU 绑定优化
- ✅ **NUMA 感知** - NUMA 架构优化（计划中）

### 性能测试

```bash
# 运行性能基准测试（计划中）
./build/bin/benchmark_tests
```

*(待添加详细的性能基准测试结果)*

## 开发指南

### 代码规范

- 遵循 **C++17** 标准
- 使用一致的代码风格（参考现有代码）
- 添加必要的注释和文档
- 为新功能添加单元测试
- 使用 RAII 管理资源
- 避免原始指针，优先使用智能指针

### 代码格式化

```bash
# 格式化所有代码
cd tools/scripts
./clang_format.sh
```

### 静态检查

```bash
# 运行静态代码检查
cd tools/scripts
./clang_tidy_check.sh
```

### 添加新的示例

```bash
# 1. 创建新的示例目录
mkdir -p zerocp_examples/my_example
cd zerocp_examples/my_example

# 2. 创建源文件和 CMakeLists.txt
touch my_example.cpp CMakeLists.txt

# 3. 编辑 CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)

add_executable(my_example my_example.cpp)

target_link_libraries(my_example
    PRIVATE
        zerocp_core
        zerocp_foundation
)
EOF

# 4. 在父目录的 CMakeLists.txt 中添加
# add_subdirectory(my_example)
```

### 添加新的组件

1. 在项目根目录创建新的组件目录
2. 创建 `include/` 和 `source/` 目录
3. 添加 `CMakeLists.txt` 配置
4. 在根 `CMakeLists.txt` 中添加子目录
5. 添加必要的文档（README.md）
6. 编写单元测试

### 工具脚本

| 脚本 | 功能 |
|------|------|
| `clang_format.sh` | 代码格式化 |
| `clang_tidy_check.sh` | 静态代码检查 |
| `lcov_generate.sh` | 生成代码覆盖率报告 |
| `check_atomic_usage.sh` | 检查原子操作使用 |
| `list_stl_dependencies.sh` | 列出 STL 依赖 |

详见 [tools/scripts/](tools/scripts/) 目录。

## 文档

### 核心文档
- [项目总体架构](docs/ARCHITECTURE.md) *(计划中)*
- [API 参考](docs/API_REFERENCE.md) *(计划中)*
- [设计决策](docs/DESIGN_DECISIONS.md) *(计划中)*

### 组件文档
- [Introspection 组件文档](zerocp_introspection/README.md)
- [Introspection 架构设计](zerocp_introspection/ARCHITECTURE.md)
- [Introspection 使用示例](zerocp_introspection/example_usage.cpp)
- [监控工具文档](tools/introspection/README.md)

### 基础库文档
- [并发库说明](zerocp_foundationLib/concurrent/README.md)
- [内存管理文档](zerocp_foundationLib/memory/README.md) *(计划中)*
- [POSIX 封装文档](zerocp_foundationLib/posix/README.md) *(计划中)*
- [静态 STL 文档](zerocp_foundationLib/staticstl/README.md) *(计划中)*

### 示例代码
- [简单发布-订阅](zerocp_examples/simple_pub_sub/)
- [高性能传输](zerocp_examples/high_perf/)

### 测试文档
- [测试总览](zerocp_introspection/test/README.md)
- [测试快速开始](zerocp_introspection/test/QUICK_START.md)

## 贡献

欢迎贡献代码和功能改进！我们非常感谢任何形式的贡献。

### 贡献流程

1. **Fork 本仓库**
2. **创建功能分支**
   ```bash
   git checkout -b feature/AmazingFeature
   ```
3. **提交更改**
   ```bash
   git commit -m 'Add some AmazingFeature'
   ```
4. **推送到分支**
   ```bash
   git push origin feature/AmazingFeature
   ```
5. **创建 Pull Request**

### 提交规范

遵循 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Type 类型：**
- `feat`: 新功能
- `fix`: 错误修复
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 代码重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具相关

**示例：**
```
feat(introspection): add CPU temperature monitoring

Add support for monitoring CPU temperature in the introspection
server. This enables better thermal management for high-load scenarios.

Closes #123
```

### 代码审查

所有提交都需要经过代码审查。审查要点：

- ✅ 代码风格一致性
- ✅ 测试覆盖率
- ✅ 文档完整性
- ✅ 性能影响
- ✅ 向后兼容性

## 许可证

本项目采用 [MIT License](LICENSE) 许可证。

```
MIT License

Copyright (c) 2025 Zero Copy Framework Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## 联系方式

- **项目主页**: <repository-url>
- **问题反馈**: [GitHub Issues](<repository-url>/issues)
- **讨论区**: [GitHub Discussions](<repository-url>/discussions)
- **邮件**: <maintainer-email> *(待添加)*

## 致谢

感谢所有为本项目做出贡献的开发者！

### 贡献者

<!-- 此处可添加贡献者列表 -->

### 灵感来源

本项目受到以下优秀开源项目的启发：

- [Eclipse iceoryx](https://github.com/eclipse-iceoryx/iceoryx) - 零拷贝中间件
- [Boost.Interprocess](https://www.boost.org/doc/libs/release/doc/html/interprocess.html) - 进程间通信库
- [ZeroMQ](https://zeromq.org/) - 高性能消息队列

---

**⭐ 如果这个项目对你有帮助，请给我们一个 Star！**
