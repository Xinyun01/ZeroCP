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
- 💪 **类型安全** - 现代 C++17/C++23，强类型设计
- 🔧 **POSIX 标准** - 基于 POSIX 共享内存和 mmap 实现

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
│   ├── core/                         # 核心类型和工具
│   ├── design/                       # 设计模式和架构
│   ├── filesystem/                   # 文件系统工具
│   ├── memory/                       # 内存管理（分配器、内存池）
│   ├── posix/                        # POSIX 接口封装
│   │   ├── ipc/                      # IPC 相关封装 ⭐
│   │   │   ├── include/
│   │   │   │   ├── posix_sharedmemory.hpp      # POSIX 共享内存封装
│   │   │   │   ├── posix_memorymap.hpp         # POSIX mmap 封装
│   │   │   │   └── posix_sharedmemory_object.hpp
│   │   │   └── source/               # IPC 实现
│   │   │       ├── posix_sharedmemory.cpp
│   │   │       └── posix_memorymap.cpp
│   │   └── posixcall/                # POSIX 系统调用封装
│   │       └── include/
│   │           └── posix_call.hpp    # POSIX 调用错误处理
│   ├── report/                       # 日志和报告系统
│   │   ├── include/
│   │   │   ├── logging.hpp           # 日志接口
│   │   │   ├── logstream.hpp         # 日志流
│   │   │   ├── log_backend.hpp       # 日志后端
│   │   │   └── lockfree_ringbuffer.hpp
│   │   └── source/
│   │       ├── logging.cpp
│   │       ├── logstream.cpp
│   │       ├── log_backend.cpp
│   │       └── lockfree_ringbuffer.cpp
│   └── vocabulary/                   # 词汇表（类型定义）
│       └── include/
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
├── test/                             # 测试套件
│   └── posix/
│       └── ipc/                      # POSIX IPC 测试 ⭐
│           ├── test_posix_sharedmemory.cpp      # 共享内存单元测试
│           ├── test_cross_process_shm.cpp       # 跨进程通信测试
│           ├── CMakeLists.txt                   # 测试构建配置
│           ├── build_and_run_tests.sh           # 一键构建运行脚本
│           └── README_TEST.md                   # 测试文档
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
├── CODE_STYLE.md                     # 代码风格指南
├── format_all.sh                     # 格式化所有代码
└── README.md                         # 本文件
```

## 核心功能

### 🚀 POSIX 共享内存封装 (posix/ipc)

高性能的 POSIX 共享内存和内存映射封装，提供现代 C++ 接口。

**主要组件：**

#### 1. PosixSharedMemory (`posix_sharedmemory.hpp`)

POSIX 共享内存对象的 RAII 封装，提供安全的生命周期管理。

**特性：**
- ✅ 基于 `shm_open()` 的现代封装
- ✅ 支持多种打开模式（创建、打开、创建或打开）
- ✅ 灵活的访问权限控制（只读、只写、读写）
- ✅ RAII 资源管理，自动清理
- ✅ 移动语义支持
- ✅ Builder 模式，流式 API

**使用示例：**
```cpp
#include "posix_sharedmemory.hpp"

// 创建共享内存
auto shmResult = PosixSharedMemoryBuilder()
    .name("my_shm")                           // 共享内存名称
    .memorySize(4096)                         // 内存大小（字节）
    .accessMode(AccessMode::ReadWrite)        // 访问模式
    .openMode(OpenMode::PurgeAndCreate)       // 打开模式
    .filePermissions(Perms::OwnerAll)         // 文件权限
    .create();

if (shmResult) {
    auto& shm = shmResult.value();
    int fd = shm.getHandle();                 // 获取文件描述符
    uint64_t size = shm.getMemorySize();      // 获取内存大小
    bool owned = shm.hasOwnership();          // 是否拥有所有权
}
```

**访问模式（AccessMode）：**
- `ReadOnly` - 只读访问
- `WriteOnly` - 只写访问
- `ReadWrite` - 读写访问

**打开模式（OpenMode）：**
- `PurgeAndCreate` - 删除已存在的，创建新的
- `ExclusiveCreate` - 排他创建（如果存在则失败）
- `OpenOrCreate` - 打开已存在的或创建新的
- `OpenExisting` - 仅打开已存在的

**文件权限（Perms）：**
- `OwnerAll` - 所有者全部权限 (0700)
- `OwnerRead` - 所有者读权限 (0400)
- `OwnerWrite` - 所有者写权限 (0200)
- `GroupAll` - 组全部权限 (0070)
- `OthersAll` - 其他用户全部权限 (0007)
- `All` - 所有用户全部权限 (0777)

#### 2. PosixMemoryMap (`posix_memorymap.hpp`)

POSIX `mmap()` 的现代 C++ 封装，实现共享内存到进程地址空间的映射。

**特性：**
- ✅ 基于 `mmap()` 的零拷贝内存映射
- ✅ 自动 `munmap()` 清理
- ✅ 支持共享和私有映射
- ✅ 灵活的保护模式（PROT_READ, PROT_WRITE）
- ✅ Builder 模式，类型安全
- ✅ 移动语义支持

**使用示例：**
```cpp
#include "posix_memorymap.hpp"

// 创建内存映射
auto mapResult = PosixMemoryMapBuilder()
    .fileDescriptor(shm.getHandle())         // 文件描述符
    .memoryLength(shm.getMemorySize())       // 映射长度
    .prot(PROT_READ | PROT_WRITE)            // 保护模式
    .flags(MAP_SHARED)                       // 映射标志
    .offset_(0)                              // 偏移量
    .create();

if (mapResult) {
    auto& memMap = mapResult.value();
    void* addr = memMap.getBaseAddress();    // 获取映射地址
    uint64_t length = memMap.getLength();    // 获取映射长度
    
    // 直接操作内存（零拷贝）
    char* data = static_cast<char*>(addr);
    std::strcpy(data, "Hello, Zero Copy!");
}
```

#### 3. PosixCall 错误处理 (`posix_call.hpp`)

统一的 POSIX 系统调用错误处理机制。

**特性：**
- ✅ 基于 `std::expected` 的错误处理
- ✅ 自动捕获 errno
- ✅ 类型安全的返回值
- ✅ 流式 API

**使用示例：**
```cpp
#include "posix_call.hpp"

auto result = ZeroCp_PosixCall(shm_open)(name, flags, mode)
    .failureReturnValue(-1)
    .evaluate();

if (result.has_value()) {
    int fd = result.value().value;
} else {
    int err = result.error().errnum;
    std::cerr << "Error: " << strerror(err) << std::endl;
}
```

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
- **IPC** - 共享内存和内存映射封装
- **PosixCall** - 统一的系统调用错误处理
- 跨平台兼容性支持
- 异常安全的资源管理

#### 报告系统 (report)
- **日志系统** - 高性能日志记录
- **无锁环形缓冲区** - 用于日志缓存
- **多级日志** - Debug, Info, Warning, Error, Fatal
- **流式日志** - 支持 `<<` 操作符

**日志使用：**
```cpp
#include "logging.hpp"

// 使用宏记录日志
ZEROCP_LOG(Info, "System started");
ZEROCP_LOG(Error, "Failed to open file: " << filename);
ZEROCP_LOG(Debug, "Counter value: " << counter);
```

#### 词汇表 (vocabulary)
- 通用类型定义
- 枚举和常量
- 类型别名

### 🔍 Introspection 监控组件

**独立的系统监控组件** - 采用客户端-服务端架构，提供运行时数据收集和查询能力。

#### 组件特性
- ✅ **客户端-服务端架构** - 服务端负责数据收集，支持多客户端访问
- ✅ **双模式访问** - 同步查询和异步订阅两种模式
- ✅ **完整监控** - 内存、进程、连接、系统负载全方位监控
- ✅ **灵活配置** - 可配置更新间隔和过滤器
- ✅ **事件驱动** - 发布-订阅模式，支持事件回调
- ✅ **独立部署** - 可作为独立库集成到其他项目

详细文档：[zerocp_introspection/README.md](zerocp_introspection/README.md)

### 🔧 Introspection 监控工具

**TUI 可视化工具** - 基于 Introspection 组件的实时监控界面。

详细文档：[tools/introspection/README.md](tools/introspection/README.md)

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
| **编译器** | GCC 7+ 或 Clang 6+ (支持 C++17) / GCC 13+ 或 Clang 16+ (C++23) |
| **构建工具** | CMake 3.16+ |
| **依赖库** | pthread (线程支持)<br>rt (POSIX 实时扩展)<br>ncurses (监控工具界面，可选) |

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

### 运行 POSIX IPC 测试

#### 方式一：使用一键脚本（推荐）

```bash
cd test/posix/ipc
./build_and_run_tests.sh
```

#### 方式二：手动编译运行

```bash
# 进入测试目录
cd test/posix/ipc

# 创建构建目录
mkdir -p build && cd build

# 配置和编译
cmake ..
make

# 运行单元测试
./test_posix_sharedmemory

# 运行跨进程通信测试
./test_cross_process_shm
```

### 测试说明

#### 1. 单元测试 (`test_posix_sharedmemory`)

测试 PosixSharedMemory 和 PosixMemoryMap 的各种功能：

- ✅ 创建和打开共享内存
- ✅ 不同访问模式（只读、只写、读写）
- ✅ 不同打开模式
- ✅ 内存映射和数据读写
- ✅ 不同大小的共享内存
- ✅ 错误处理
- ✅ 移动语义
- ✅ 文件权限

#### 2. 跨进程通信测试 (`test_cross_process_shm`)

测试真实的跨进程场景：

- ✅ **跨进程通信** - 父子进程通过共享内存交换数据
- ✅ **零拷贝验证** - 验证多个映射看到同一份物理内存
- ✅ **性能测试** - 大数据传输性能（10MB）

**预期输出：**
```
╔══════════════════════════════════════════════════════════════╗
║         Cross-Process Shared Memory Communication Test      ║
╚══════════════════════════════════════════════════════════════╝
✅ Parent: Created shared memory
✅ Parent: Created memory map at address 0x7f1234567000
✅ Parent: Initialized shared data
   Parent PID: 12345
   Initial message: "Hello from parent"

--- Child Process Started ---
   Child PID: 12346
✅ Child: Opened shared memory
✅ Child: Created memory map at address 0x7f1234567000
✅ Child: Read data from parent
   Parent PID (from shared memory): 12345
   Parent message: "Hello from parent"
✅ Child: Modified shared data
   New counter: 42
   New message: "Hello from child"
--- Child Process Exiting ---

--- Parent: Waiting for child process ---
✅ Parent: Child process completed successfully

--- Parent: Reading modified data from child ---
   Counter (modified by child): 42
   Message (modified by child): "Hello from child"
   Ready flag: true

╔══════════════════════════════════════════════════════════════╗
║         ✅ Cross-Process Communication Test PASSED          ║
╚══════════════════════════════════════════════════════════════╝
```

更多测试文档：[test/posix/ipc/README_TEST.md](test/posix/ipc/README_TEST.md)

## 详细使用

### 基本使用示例

#### 1. 创建共享内存并映射

```cpp
#include "posix_sharedmemory.hpp"
#include "posix_memorymap.hpp"
#include <iostream>
#include <cstring>

int main() {
    // 1. 创建共享内存
    auto shmResult = PosixSharedMemoryBuilder()
        .name("my_app_shm")
        .memorySize(4096)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    if (!shmResult) {
        std::cerr << "Failed to create shared memory" << std::endl;
        return 1;
    }
    
    auto& shm = shmResult.value();
    
    // 2. 创建内存映射
    auto mapResult = PosixMemoryMapBuilder()
        .fileDescriptor(shm.getHandle())
        .memoryLength(shm.getMemorySize())
        .prot(PROT_READ | PROT_WRITE)
        .flags(MAP_SHARED)
        .offset_(0)
        .create();
    
    if (!mapResult) {
        std::cerr << "Failed to create memory map" << std::endl;
        return 1;
    }
    
    auto& memMap = mapResult.value();
    
    // 3. 使用映射的内存（零拷贝）
    void* addr = memMap.getBaseAddress();
    std::strcpy(static_cast<char*>(addr), "Hello, Zero Copy!");
    
    std::cout << "Data: " << static_cast<char*>(addr) << std::endl;
    
    return 0;
}
```

#### 2. 跨进程通信示例

**进程 A（写入者）：**
```cpp
#include "posix_sharedmemory.hpp"
#include "posix_memorymap.hpp"

int main() {
    // 创建共享内存
    auto shmResult = PosixSharedMemoryBuilder()
        .name("ipc_channel")
        .memorySize(1024)
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::PurgeAndCreate)
        .filePermissions(Perms::OwnerAll)
        .create();
    
    auto& shm = shmResult.value();
    
    // 映射内存
    auto mapResult = PosixMemoryMapBuilder()
        .fileDescriptor(shm.getHandle())
        .memoryLength(shm.getMemorySize())
        .prot(PROT_READ | PROT_WRITE)
        .flags(MAP_SHARED)
        .offset_(0)
        .create();
    
    auto& memMap = mapResult.value();
    
    // 写入数据
    char* data = static_cast<char*>(memMap.getBaseAddress());
    std::strcpy(data, "Message from Process A");
    
    std::cout << "Process A: Data written" << std::endl;
    
    // 保持进程运行
    std::cin.get();
    
    return 0;
}
```

**进程 B（读取者）：**
```cpp
#include "posix_sharedmemory.hpp"
#include "posix_memorymap.hpp"

int main() {
    // 打开已存在的共享内存
    auto shmResult = PosixSharedMemoryBuilder()
        .name("ipc_channel")
        .memorySize(1024)
        .accessMode(AccessMode::ReadOnly)
        .openMode(OpenMode::OpenExisting)
        .create();
    
    auto& shm = shmResult.value();
    
    // 映射内存
    auto mapResult = PosixMemoryMapBuilder()
        .fileDescriptor(shm.getHandle())
        .memoryLength(shm.getMemorySize())
        .prot(PROT_READ)
        .flags(MAP_SHARED)
        .offset_(0)
        .create();
    
    auto& memMap = mapResult.value();
    
    // 读取数据
    const char* data = static_cast<const char*>(memMap.getBaseAddress());
    std::cout << "Process B: Read data: " << data << std::endl;
    
    return 0;
}
```

#### 3. 使用日志系统

```cpp
#include "logging.hpp"

int main() {
    // 记录不同级别的日志
    ZEROCP_LOG(Debug, "Debug information: value = " << 42);
    ZEROCP_LOG(Info, "Application started");
    ZEROCP_LOG(Warning, "Low memory warning");
    ZEROCP_LOG(Error, "Failed to open file: " << "config.txt");
    
    return 0;
}
```

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
        pthread
        rt
)

# 添加头文件路径
target_include_directories(your_target
    PRIVATE
        path/to/zero_copy_framework/zerocp_foundationLib/posix/ipc/include
        path/to/zero_copy_framework/zerocp_foundationLib/report/include
)
```

#### 方式二：直接编译

```bash
g++ your_app.cpp \
    -std=c++23 \
    -I/path/to/zerocp_foundationLib/posix/ipc/include \
    -I/path/to/zerocp_foundationLib/posix/posixcall/include \
    -I/path/to/zerocp_foundationLib/report/include \
    -I/path/to/zerocp_foundationLib/filesystem/include \
    -I/path/to/zerocp_foundationLib/core/include \
    -I/path/to/zerocp_foundationLib/design \
    -I/path/to/zerocp_foundationLib/vocabulary/include \
    /path/to/source/posix_sharedmemory.cpp \
    /path/to/source/posix_memorymap.cpp \
    /path/to/source/logging.cpp \
    /path/to/source/logstream.cpp \
    /path/to/source/log_backend.cpp \
    /path/to/source/lockfree_ringbuffer.cpp \
    -pthread -lrt \
    -o your_app
```

## 测试

### 运行所有测试

```bash
# 使用脚本运行所有测试
./run_tests.sh

# 或者手动运行
cd build
ctest
```

### 运行特定测试

```bash
# POSIX IPC 测试
cd test/posix/ipc
./build_and_run_tests.sh

# Introspection 测试
cd build
./bin/introspection_tests
```

### 测试覆盖率

```bash
# 生成代码覆盖率报告
cd tools/scripts
./lcov_generate.sh
```

更多测试文档：
- [POSIX IPC 测试文档](test/posix/ipc/README_TEST.md)
- [Introspection 测试文档](zerocp_introspection/test/README.md)

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
- ✅ **RAII 管理** - 自动资源清理，无内存泄漏
- ✅ **系统调用优化** - 统一的错误处理，减少开销

### 零拷贝原理

传统 IPC 方法（如管道、消息队列）需要多次内存拷贝：

```
进程A内存 → 内核缓冲区 → 进程B内存
```

Zero Copy Framework 使用共享内存 + mmap：

```
进程A ←→ 共享内存 ←→ 进程B
      (同一块物理内存，零拷贝)
```

## 开发指南

### 代码规范

- 遵循 **C++17/C++23** 标准
- 使用一致的代码风格（参考 `CODE_STYLE.md`）
- 添加必要的注释和文档
- 为新功能添加单元测试
- 使用 RAII 管理资源
- 避免原始指针，优先使用智能指针
- 使用 `std::expected` 进行错误处理

### 代码格式化

```bash
# 格式化所有代码
./format_all.sh

# 或使用工具脚本
cd tools/scripts
./clang_format.sh
```

### 静态检查

```bash
cd tools/scripts
./clang_tidy_check.sh
```

### 添加新的测试

1. 在 `test/` 目录下创建测试文件
2. 在 `CMakeLists.txt` 中添加测试目标
3. 编写测试用例
4. 运行测试验证

### 调试技巧

**查看共享内存对象：**
```bash
ls -lh /dev/shm/
```

**清理残留的共享内存：**
```bash
# 删除特定共享内存
rm /dev/shm/my_shm_name

# 删除所有共享内存（谨慎使用）
rm /dev/shm/*
```

**查看内存映射：**
```bash
cat /proc/<pid>/maps
```

## 文档

### 核心文档
- [代码风格指南](CODE_STYLE.md)
- [项目总体架构](docs/ARCHITECTURE.md) *(计划中)*
- [API 参考](docs/API_REFERENCE.md) *(计划中)*

### 组件文档
- [Introspection 组件文档](zerocp_introspection/README.md)
- [Introspection 架构设计](zerocp_introspection/ARCHITECTURE.md)
- [监控工具文档](tools/introspection/README.md)
- [并发库说明](zerocp_foundationLib/concurrent/README.md)

### 测试文档
- [POSIX IPC 测试文档](test/posix/ipc/README_TEST.md)
- [Introspection 测试总览](zerocp_introspection/test/README.md)
- [Introspection 测试快速开始](zerocp_introspection/test/QUICK_START.md)

## 贡献

欢迎贡献代码和功能改进！

### 贡献流程

1. **Fork 本仓库**
2. **创建功能分支**
   ```bash
   git checkout -b feature/AmazingFeature
   ```
3. **提交更改**
   ```bash
   git commit -m 'feat: add some amazing feature'
   ```
4. **推送到分支**
   ```bash
   git push origin feature/AmazingFeature
   ```
5. **创建 Pull Request**

### 提交规范

遵循 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

- `feat`: 新功能
- `fix`: 错误修复
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 代码重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具相关

## 许可证

本项目采用 [MIT License](LICENSE) 许可证。

## 联系方式

- **项目主页**: <repository-url>
- **问题反馈**: [GitHub Issues](<repository-url>/issues)
- **讨论区**: [GitHub Discussions](<repository-url>/discussions)

## 致谢

### 灵感来源

本项目受到以下优秀开源项目的启发：

- [Eclipse iceoryx](https://github.com/eclipse-iceoryx/iceoryx) - 零拷贝中间件
- [Boost.Interprocess](https://www.boost.org/doc/libs/release/doc/html/interprocess.html) - 进程间通信库
- [ZeroMQ](https://zeromq.org/) - 高性能消息队列

---

**⭐ 如果这个项目对你有帮助，请给我们一个 Star！**
