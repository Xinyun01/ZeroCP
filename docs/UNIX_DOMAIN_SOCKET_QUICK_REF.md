# Unix Domain Socket 快速参考

## 🚀 快速开始

### 服务端 (3 步)

```cpp
// 1. 创建服务端
auto server = UnixDomainSocketBuilder()
    .path("/tmp/app.sock")
    .socketType(SocketType::Stream)
    .createServer()
    .value();

// 2. 接受连接
auto client = server.accept().value();

// 3. 通信
char buf[256];
client.receive(buf, sizeof(buf));
client.send("response", 8);
```

### 客户端 (2 步)

```cpp
// 1. 连接服务端
auto client = UnixDomainSocketBuilder()
    .path("/tmp/app.sock")
    .socketType(SocketType::Stream)
    .createClient()
    .value();

// 2. 通信
client.send("request", 7);
char buf[256];
client.receive(buf, sizeof(buf));
```

## 📋 关键系统调用顺序

```
服务端: socket() → bind() → listen() → accept() → send()/recv() → close() → unlink()
客户端: socket() → connect() → send()/recv() → close()
```

## 🔧 Builder 配置选项

```cpp
UnixDomainSocketBuilder()
    .path("/tmp/my.sock")           // ✅ 必需：socket 路径 (< 108 字节)
    .socketType(SocketType::Stream) // 可选：Stream(默认) 或 Datagram
    .backlog(10)                    // 可选：监听队列长度（仅服务端）
    .filePermissions(Perms::OwnerAll) // 可选：文件权限 (0700)
    .createServer()                 // 或 .createClient()
```

## 📊 Socket 类型对比

| 特性 | SOCK_STREAM | SOCK_DGRAM |
|------|-------------|------------|
| **连接** | 面向连接 | 无连接 |
| **可靠性** | 可靠 | 不可靠 |
| **顺序** | 保证 | 不保证 |
| **消息边界** | 无 | 有 |
| **类似于** | TCP | UDP |
| **适用场景** | 可靠传输 | 快速消息 |

## 🔑 常用 API

### UnixDomainSocket

```cpp
// 数据传输
ssize_t send(const void* data, size_t size);
ssize_t receive(void* buffer, size_t size);

// 状态查询
int getFileDescriptor();
bool isConnected();
const std::string& getPath();

// 资源管理
void close();
```

### UnixDomainSocketServer

```cpp
// 接受连接
std::expected<UnixDomainSocket, UnixDomainSocketError> accept();

// 查询信息
int getFileDescriptor();
const std::string& getPath();

// 资源管理
void close();
```

## 🎯 常见模式

### 模式 1: 简单请求-响应

```cpp
// 服务端
auto server = builder.createServer().value();
auto client = server.accept().value();
char req[256];
client.receive(req, sizeof(req));
client.send("ACK", 3);

// 客户端
auto client = builder.createClient().value();
client.send("REQ", 3);
char resp[256];
client.receive(resp, sizeof(resp));
```

### 模式 2: 持续通信

```cpp
// 服务端
while (true) {
    char buf[256];
    ssize_t n = client.receive(buf, sizeof(buf));
    if (n <= 0) break;  // 客户端断开
    // 处理数据...
    client.send(response, len);
}
```

### 模式 3: 多客户端

```cpp
// 服务端
while (true) {
    auto client = server.accept().value();
    std::thread([c = std::move(client)]() mutable {
        // 处理客户端...
    }).detach();
}
```

### 模式 4: 父子进程通信

```cpp
pid_t pid = fork();
if (pid == 0) {
    // 子进程：服务端
    auto server = builder.createServer().value();
    auto client = server.accept().value();
    // ...
} else {
    // 父进程：客户端
    sleep(1);  // 等待服务端启动
    auto client = builder.createClient().value();
    // ...
}
```

## ⚠️ 常见错误及解决

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| **Address already in use** | socket 文件已存在 | 服务端会自动 unlink，或手动 `rm /tmp/*.sock` |
| **Permission denied** | 权限不足 | 使用 `.filePermissions(Perms::OwnerAll)` |
| **No such file** | 服务端未启动 | 先启动服务端，客户端延迟连接 |
| **Connection refused** | 服务端未 listen | 确保调用 `listen()` (Stream 模式) |
| **Path too long** | 路径超过 108 字节 | 使用短路径 |
| **发送/接收部分数据** | 缓冲区未满/未空 | 循环调用直到完成 |

## 🛠️ 调试技巧

```bash
# 查看 socket 文件
ls -l /tmp/*.sock

# 查看进程打开的 socket
lsof | grep my.sock

# 跟踪系统调用
strace -e trace=socket,bind,connect,send,recv ./app

# 清理所有 socket 文件
rm /tmp/*.sock
```

## 📈 性能优化

```cpp
// 1. 使用大缓冲区
const size_t BUFFER_SIZE = 64 * 1024;  // 64 KB

// 2. 批量发送
std::vector<char> data(large_size);
sendAll(socket, data.data(), data.size());

// 3. 设置 socket 选项
int optval = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
```

## 📚 完整示例

### echo 服务器

```cpp
#include "unix_domainsocket.hpp"
#include <iostream>

int main() {
    auto server = UnixDomainSocketBuilder()
        .path("/tmp/echo.sock")
        .createServer()
        .value();
    
    std::cout << "Echo server running..." << std::endl;
    
    while (true) {
        auto client = server.accept().value();
        
        char buffer[256];
        while (true) {
            ssize_t n = client.receive(buffer, sizeof(buffer) - 1);
            if (n <= 0) break;
            
            buffer[n] = '\0';
            std::cout << "Received: " << buffer << std::endl;
            
            client.send(buffer, n);  // echo back
        }
    }
}
```

### echo 客户端

```cpp
#include "unix_domainsocket.hpp"
#include <iostream>
#include <string>

int main() {
    auto client = UnixDomainSocketBuilder()
        .path("/tmp/echo.sock")
        .createClient()
        .value();
    
    std::string line;
    while (std::getline(std::cin, line)) {
        client.send(line.c_str(), line.length());
        
        char buffer[256];
        ssize_t n = client.receive(buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            std::cout << "Echo: " << buffer << std::endl;
        }
    }
}
```

## 🔗 相关链接

- [详细指南](../test/posix/ipc/UNIX_DOMAIN_SOCKET_GUIDE.md) - 完整使用指南
- [流程详解](UNIX_DOMAIN_SOCKET_FLOW.md) - 内核层面的详细流程
- [测试代码](../test/posix/ipc/test_unix_domainsocket.cpp) - 完整测试示例
- [API 头文件](../zerocp_foundationLib/posix/ipc/include/unix_domainsocket.hpp) - 接口定义

## 📝 权限参考

```cpp
// 常用权限组合
Perms::OwnerAll              // 0700 - 仅所有者
Perms::OwnerRead | Perms::OwnerWrite  // 0600 - 所有者读写
Perms::OwnerAll | Perms::GroupAll     // 0770 - 所有者和组
Perms::All                   // 0777 - 所有人（不推荐）
```

## 💾 编译命令

```bash
# 使用 CMake
cd test/posix/ipc/build
cmake .. && make test_unix_domainsocket
./test_unix_domainsocket

# 手动编译
g++ -std=c++23 your_app.cpp \
    -I.../posix/ipc/include \
    .../unix_domainsocket.cpp \
    -pthread -o your_app
```

---

**快速参考结束** | 更多详情请查看完整文档

