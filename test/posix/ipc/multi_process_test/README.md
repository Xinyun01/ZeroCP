# 多进程 Unix Domain Socket 通信测试

## 📋 项目概述

这是一个完整的多进程 Unix Domain Socket (UDS) 通信测试项目，演示了如何使用 `SOCK_DGRAM`（数据报）模式实现一对多的客户端-服务端通信。

### 特性

- ✅ **多客户端并发通信**：支持多个客户端同时与服务端通信
- ✅ **无连接模式**：使用 `SOCK_DGRAM` 类型，无需建立连接
- ✅ **双向通信**：客户端和服务端可以互相发送和接收消息
- ✅ **完整的测试框架**：提供自动化编译、运行和测试脚本
- ✅ **详细日志**：每个进程都有独立的日志输出

## 🏗️ 架构设计

### 通信模型

```
┌─────────────┐
│  Client 1   │──┐
│ (PID: xxx)  │  │
└─────────────┘  │
                 │      ┌──────────────┐
┌─────────────┐  │      │              │
│  Client 2   │──┼─────▶│   Server     │
│ (PID: yyy)  │  │      │  (PID: zzz)  │
└─────────────┘  │      │              │
                 │      └──────────────┘
┌─────────────┐  │
│  Client 3   │──┘
│ (PID: www)  │
└─────────────┘
```

### Socket 文件

| 进程 | Socket 路径 | 角色 |
|------|------------|------|
| Server | `/tmp/uds_multi_process_server.sock` | 服务端，接收所有客户端消息 |
| Client 1 | `/tmp/uds_client_1.sock` | 客户端 1，绑定本地地址以接收响应 |
| Client 2 | `/tmp/uds_client_2.sock` | 客户端 2，绑定本地地址以接收响应 |
| Client 3 | `/tmp/uds_client_3.sock` | 客户端 3，绑定本地地址以接收响应 |

### 通信流程

1. **服务端启动**
   - 创建 `SOCK_DGRAM` 套接字
   - 绑定到 `/tmp/uds_multi_process_server.sock`
   - 进入接收循环，等待客户端消息

2. **客户端启动**（并发启动 3 个客户端）
   - 创建 `SOCK_DGRAM` 套接字
   - 绑定到各自的本地地址（用于接收服务端响应）
   - 向服务端发送消息

3. **消息交换**
   - 客户端使用 `sendTo()` 发送消息到服务端
   - 服务端使用 `receiveFrom()` 接收消息并记录发送者地址
   - 服务端使用 `sendTo()` 回复给发送者
   - 客户端使用 `receiveFrom()` 接收服务端响应

## 📁 项目结构

```
multi_process_test/
├── config.hpp          # 共享配置（socket 路径、常量等）
├── server.cpp          # 服务端实现
├── client1.cpp         # 客户端 1 实现
├── client2.cpp         # 客户端 2 实现
├── client3.cpp         # 客户端 3 实现
├── CMakeLists.txt      # CMake 构建配置
├── build.sh            # 编译脚本
├── run_test.sh         # 运行测试脚本
├── test.sh             # 一键编译+运行脚本
└── README.md           # 本文档
```

## 🚀 快速开始

### 前置要求

- **操作系统**: Linux（或支持 Unix Domain Socket 的类 Unix 系统）
- **编译器**: GCC 10+ 或 Clang 12+（支持 C++23）
- **CMake**: 3.16 或更高版本
- **依赖库**: ZeroCP Foundation Library

### 一键运行

```bash
# 编译并运行测试（Release 模式）
./test.sh

# 或者 Debug 模式
./test.sh debug
```

### 分步运行

#### 1. 编译

```bash
# Release 模式
./build.sh

# Debug 模式
./build.sh debug
```

编译成功后会在 `build/` 目录生成以下可执行文件：
- `uds_server`
- `uds_client1`
- `uds_client2`
- `uds_client3`

#### 2. 运行测试

```bash
./run_test.sh
```

此脚本会：
1. 清理旧的 socket 文件
2. 启动服务端
3. 启动 3 个客户端（并发）
4. 等待所有进程完成
5. 显示所有日志
6. 输出测试结果

#### 3. 手动运行

如果需要手动控制：

```bash
cd build/

# 终端 1: 启动服务端
./uds_server

# 终端 2: 启动客户端 1
./uds_client1

# 终端 3: 启动客户端 2
./uds_client2

# 终端 4: 启动客户端 3
./uds_client3
```

## 📊 配置说明

所有配置都在 `config.hpp` 中定义：

```cpp
namespace TestConfig
{
    // 服务端 socket 路径
    constexpr const char* SERVER_SOCKET_PATH = "/tmp/uds_multi_process_server.sock";
    
    // 客户端 socket 路径前缀
    constexpr const char* CLIENT_SOCKET_PREFIX = "/tmp/uds_client_";
    
    // 最大消息大小
    constexpr uint64_t MAX_MESSAGE_SIZE = 256;
    
    // 每个客户端发送的消息数量
    constexpr int MESSAGES_PER_CLIENT = 5;
    
    // 客户端启动延迟（毫秒）
    constexpr int CLIENT_STARTUP_DELAY_MS = 500;
    
    // 消息发送间隔（毫秒）
    constexpr int MESSAGE_INTERVAL_MS = 100;
    
    // 客户端数量
    constexpr int NUM_CLIENTS = 3;
}
```

## 🧪 测试场景

### 场景 1: 并发消息发送

每个客户端发送 5 条消息到服务端，消息格式：

```
Hello from Client-X, message #N
```

服务端接收所有消息并回复：

```
Server received: [original message]
```

### 场景 2: 消息验证

客户端验证接收到的响应是否包含原始消息内容。

### 场景 3: 进程清理

测试结束后，自动清理所有进程和 socket 文件。

## 📝 日志输出

测试运行时，会生成以下日志文件（在 `build/` 目录）：

- `server.log` - 服务端日志
- `client1.log` - 客户端 1 日志
- `client2.log` - 客户端 2 日志
- `client3.log` - 客户端 3 日志

### 示例日志

**服务端日志：**
```
[INFO] Server created successfully
[INFO] Server listening on: /tmp/uds_multi_process_server.sock
[INFO] Received message from: /tmp/uds_client_1.sock
[INFO] Message: Hello from Client-1, message #1
[INFO] Sent reply to: /tmp/uds_client_1.sock
```

**客户端日志：**
```
[INFO] Client created successfully
[INFO] Client bound to: /tmp/uds_client_1.sock
[INFO] Sending message #1 to server
[INFO] Received response: Server received: Hello from Client-1, message #1
```

## 🔧 故障排查

### 问题 1: 编译失败

**症状**: `build.sh` 报错

**解决方案**:
1. 检查 C++23 编译器是否安装
2. 检查 CMake 版本是否 >= 3.16
3. 检查 ZeroCP Foundation Library 路径是否正确

### 问题 2: Socket 文件已存在

**症状**: `bind() failed: Address already in use`

**解决方案**:
```bash
# 清理 socket 文件
rm -f /tmp/uds_multi_process_server.sock
rm -f /tmp/uds_client_*.sock
```

### 问题 3: 客户端无法连接服务端

**症状**: 客户端发送消息失败

**解决方案**:
1. 确保服务端先启动
2. 检查服务端是否正在运行
3. 检查 socket 文件权限

### 问题 4: 进程未清理

**症状**: 测试结束后进程仍在运行

**解决方案**:
```bash
# 查找并杀死所有相关进程
pkill -f uds_server
pkill -f uds_client
```

## 🎯 关键技术点

### 1. SOCK_DGRAM vs SOCK_STREAM

| 特性 | SOCK_DGRAM | SOCK_STREAM |
|-----|-----------|-------------|
| 连接 | 无连接 | 面向连接 |
| 可靠性 | 不保证 | 可靠 |
| 消息边界 | 保留 | 不保留 |
| API | `sendTo/receiveFrom` | `send/recv` |
| 适用场景 | 简单请求-响应 | 持续数据流 |

### 2. 客户端为何需要 bind()?

在 `SOCK_DGRAM` 模式下，客户端需要绑定本地地址的原因：

1. **接收响应**：服务端需要知道回复给谁
2. **识别发送者**：`receiveFrom()` 返回发送者地址
3. **双向通信**：客户端和服务端都可以发送和接收

### 3. CLIENT vs SERVER 模式

虽然 `SOCK_DGRAM` 中客户端和服务端都需要绑定地址，但使用不同的 `ChannelSide` 有以下好处：

- **语义清晰**：明确角色定位
- **日志区分**：便于调试和问题追踪
- **未来扩展**：可能有不同的内部行为

### 4. 线程安全

当前实现是单线程的，如果需要多线程：

- 使用 `std::mutex` 保护共享资源
- 考虑使用线程池处理客户端请求
- 注意 `receiveFrom/sendTo` 的并发安全性

## 📚 扩展阅读

### Unix Domain Socket 相关

- `man 7 unix` - Unix domain sockets
- `man 2 socket` - Socket system call
- `man 2 bind` - Bind a name to a socket
- `man 2 sendto` - Send a message on a socket
- `man 2 recvfrom` - Receive a message from a socket

### POSIX IPC

- `man 7 shm_overview` - POSIX shared memory
- `man 7 mq_overview` - POSIX message queues
- `man 7 sem_overview` - POSIX semaphores

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

本项目遵循 ZeroCP Framework 的许可证。

## 📧 联系方式

如有问题，请联系项目维护者。

---

**最后更新时间**: 2025-10-21

