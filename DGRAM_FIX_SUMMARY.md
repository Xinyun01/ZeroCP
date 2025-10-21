# Unix Domain Socket DGRAM 模式修复总结

## 🐛 发现的问题

### 问题 1: Socket 类型不匹配
**位置**: `unix_domainsocket.cpp:34`
```cpp
// ❌ 错误：使用了 SOCK_STREAM
auto socketcall = ZeroCp_PosixCall(socket)(sockAddr.sun_family, SOCK_STREAM, 0)
```

**问题**：创建套接字时使用了 `SOCK_STREAM`（流式套接字），但实际需要 `SOCK_DGRAM`（数据报套接字）。

---

### 问题 2: send() 系统调用错误
**位置**: `unix_domainsocket.cpp:178-188`
```cpp
// ❌ 错误：DGRAM 应该使用 sendto()
auto sendCall = ZeroCp_PosixCall(send)(m_socketFd, msg.c_str(), msg.size())
```

**问题**：
- `send()` 用于 **SOCK_STREAM**（面向连接）
- `sendto()` 用于 **SOCK_DGRAM**（无连接，需要指定目标地址）

---

### 问题 3: receive() 系统调用错误
**位置**: `unix_domainsocket.cpp:190-222`
```cpp
// ❌ 错误：DGRAM 应该使用 recvfrom()
auto recvCall = ZeroCp_PosixCall(recv)(m_socketFd, buffer.data(), buffer.size(), 0)
```

**问题**：
- `recv()` 用于 **SOCK_STREAM**
- `recvfrom()` 用于 **SOCK_DGRAM**（需要获取发送者地址）
- **服务端必须保存客户端地址才能回复**

---

## ✅ 修复方案

### 修复 1: 改用 SOCK_DGRAM
**文件**: `zerocp_foundationLib/posix/ipc/source/unix_domainsocket.cpp`
**行号**: 34

```cpp
// ✅ 正确：使用 SOCK_DGRAM
auto socketcall = ZeroCp_PosixCall(socket)(sockAddr.sun_family, SOCK_DGRAM, 0)
                                        .failureReturnValue(UnixDomainSocket::ERROR_CODE)
                                        .evaluate();
```

**影响**：
- ✅ 支持无连接通信
- ✅ 保留消息边界
- ✅ 适合请求-响应模式

---

### 修复 2: 使用 sendto() 发送
**文件**: `zerocp_foundationLib/posix/ipc/source/unix_domainsocket.cpp`
**行号**: 178-193

```cpp
std::expected<void, PosixIpcChannelError> UnixDomainSocket::send(const std::string& msg) const noexcept
{
    // ✅ SOCK_DGRAM 模式：使用 sendto()
    // 对于已 connect() 的套接字，可以传递 NULL 作为目标地址
    // 对于服务端回复，需要使用上次 recvfrom() 收到的客户端地址
    auto sendCall = ZeroCp_PosixCall(sendto)(m_socketFd, msg.c_str(), msg.size(), 0,
                                             reinterpret_cast<const struct sockaddr*>(&m_sockAddr_un),
                                             sizeof(m_sockAddr_un))
                                        .failureReturnValue(UnixDomainSocket::ERROR_CODE)
                                        .evaluate();
    if (!sendCall.has_value())
    {
        return std::unexpected(UnixDomainSocket::errnoToEnum(m_name, sendCall.error().errnum));
    }
    return {};
}
```

**关键点**：
- 使用 `sendto()` 而不是 `send()`
- 使用 `m_sockAddr_un` 作为目标地址
- 对于客户端：`m_sockAddr_un` 是服务端地址（connect 时设置）
- 对于服务端：`m_sockAddr_un` 是客户端地址（recvfrom 更新）

---

### 修复 3: 使用 recvfrom() 接收并保存发送者地址
**文件**: `zerocp_foundationLib/posix/ipc/source/unix_domainsocket.cpp`
**行号**: 195-237

```cpp
std::expected<std::string, PosixIpcChannelError> UnixDomainSocket::receive(std::string& msg) const noexcept
{
    // 为接收缓冲区分配空间（使用 maxMsgSize）
    std::vector<char> buffer(m_maxMsgSize);
    
    // ✅ SOCK_DGRAM 模式：使用 recvfrom() 接收数据报
    // 重要：保存发送者地址，以便后续用 sendto() 回复
    sockaddr_un fromAddr;
    socklen_t fromLen = sizeof(fromAddr);
    memset(&fromAddr, 0, sizeof(fromAddr));
    
    auto recvCall = ZeroCp_PosixCall(recvfrom)(m_socketFd, buffer.data(), buffer.size(), 0,
                                               reinterpret_cast<struct sockaddr*>(&fromAddr),
                                               &fromLen)
                                        .failureReturnValue(UnixDomainSocket::ERROR_CODE)
                                        .evaluate();
    
    if (!recvCall.has_value())
    {
        return std::unexpected(UnixDomainSocket::errnoToEnum(m_name, recvCall.error().errnum));
    }
    
    ssize_t bytesReceived = recvCall.value();
    
    if (bytesReceived == 0)
    {
        ZEROCP_LOG(Debug, "Received empty datagram for Unix Domain Socket \"" << m_name.c_str() << "\"");
        msg.clear();
        return msg;
    }
    
    msg.assign(buffer.data(), bytesReceived);
    
    // ✅ 【关键】保存发送者地址，send() 需要用它来回复
    m_sockAddr_un = fromAddr;
    
    return msg;
}
```

**关键点**：
- 使用 `recvfrom()` 获取消息和发送者地址
- **必须保存发送者地址**到 `m_sockAddr_un`
- 服务端需要这个地址才能用 `sendto()` 回复客户端

---

### 修复 4: 将 m_sockAddr_un 标记为 mutable
**文件**: `zerocp_foundationLib/posix/ipc/include/unix_domainsocket.hpp`
**行号**: 125

```cpp
UdsName_t m_name;
PosixIpcChannelSide m_channelSide {PosixIpcChannelSide::CLIENT};
int32_t m_socketFd {INVALID_FD};
mutable sockaddr_un m_sockAddr_un {};  // ✅ mutable: receive() 需要更新客户端地址
uint64_t m_maxMsgSize {MAX_MESSAGE_SIZE};
```

**原因**：
- `receive()` 是 `const` 方法（不修改对象状态的语义）
- 但需要更新 `m_sockAddr_un` 来保存发送者地址
- 使用 `mutable` 允许在 `const` 方法中修改此成员

---

## 📊 SOCK_STREAM vs SOCK_DGRAM 对比

| 特性 | SOCK_STREAM (修复前) | SOCK_DGRAM (修复后) |
|------|---------------------|-------------------|
| **连接** | ✅ 面向连接 | ❌ 无连接 |
| **系统调用** | `send()` / `recv()` | `sendto()` / `recvfrom()` |
| **消息边界** | ❌ 字节流，无边界 | ✅ 保留边界 |
| **地址管理** | `connect()` 后自动 | 需要手动管理 |
| **服务端** | 需要 `listen()/accept()` | 直接 `bind()` 即可 |
| **复杂度** | ⚠️ 高（需要管理连接） | ✅ 低（无连接） |
| **适用场景** | 长连接、流式传输 | ✅ **请求-响应、查询** |

---

## 🔄 通信流程对比

### 修复前 (SOCK_STREAM - 错误)
```
Server                     Client
  |                          |
  | socket(STREAM) ❌         | socket(STREAM) ❌
  | bind()                   |
  | listen() ❌              |
  | accept() ❌              | connect()
  |<-------- 建立连接 -------->|
  | recv() ❌                 | send() ❌
  | send() ❌                 | recv() ❌
```

### 修复后 (SOCK_DGRAM - 正确)
```
Server                     Client
  |                          |
  | socket(DGRAM) ✅         | socket(DGRAM) ✅
  | bind()                   | connect() (可选，设置默认目标)
  |                          |
  | recvfrom() ✅            | sendto() ✅
  |   (获取客户端地址)        |   (发送到服务端)
  |                          |
  | sendto(客户端地址) ✅    | recvfrom() ✅
  |   (回复到客户端)          |   (接收响应)
```

---

## 🎯 关键理解

### 1. connect() 在 DGRAM 中的作用
```cpp
// 客户端 connect() 的作用（DGRAM 模式）：
connect(sockfd, server_addr, ...);  // 设置"默认目标地址"

// 之后可以：
send(sockfd, data, ...);           // 发送到默认目标（服务端）
// 或
sendto(sockfd, data, ..., NULL);   // NULL 表示使用默认目标
```

**注意**：DGRAM 的 `connect()` **不建立真正的连接**，只是：
- 设置默认目标地址
- 过滤只接收来自该地址的消息
- 允许使用 `send()` 而不必每次指定地址

### 2. 服务端回复机制
```cpp
// 服务端必须保存客户端地址
recvfrom(sockfd, buffer, ..., &client_addr, ...);  // 获取客户端地址
m_sockAddr_un = client_addr;                        // ✅ 保存！

// 回复时使用保存的地址
sendto(sockfd, response, ..., &m_sockAddr_un, ...); // 发送到客户端
```

### 3. 为什么需要 mutable？
```cpp
// receive() 是 const 方法（不改变对象"逻辑状态"）
std::expected<std::string, PosixIpcChannelError> receive(std::string& msg) const noexcept;
//                                                                           ^^^^^ const

// 但需要更新地址（这是"实现细节"，不是"逻辑状态"）
mutable sockaddr_un m_sockAddr_un;  // ✅ 允许在 const 方法中修改
```

---

## ✅ 验证清单

- [x] Socket 创建使用 `SOCK_DGRAM`
- [x] `send()` 改为 `sendto()`
- [x] `receive()` 改为 `recvfrom()`
- [x] 服务端保存客户端地址
- [x] `m_sockAddr_un` 标记为 `mutable`
- [x] 客户端使用 `connect()` 设置默认目标
- [x] 编译通过，无 linter 错误

---

## 📝 测试建议

### 基础测试
```bash
# 终端 1: 启动服务端
./examples/daemon_shm_server

# 终端 2: 运行客户端
./examples/client_shm_query

# 终端 3, 4, 5: 多个客户端并发
./examples/client_shm_query
./examples/client_shm_query
./examples/client_shm_query
```

### 验证点
1. ✅ 服务端能收到客户端消息
2. ✅ 客户端能收到服务端响应
3. ✅ 多个客户端可以同时查询
4. ✅ 消息内容完整（边界保留）
5. ✅ 无连接建立/关闭开销

---

## 🚀 性能影响

| 指标 | SOCK_STREAM | SOCK_DGRAM |
|------|-------------|------------|
| **连接开销** | ⚠️ 高 (listen/accept) | ✅ 无 |
| **单次请求延迟** | ⚠️ 中等 | ✅ 低 |
| **多客户端支持** | ⚠️ 需要多线程/epoll | ✅ 天然支持 |
| **代码复杂度** | ⚠️ 高 | ✅ 低 |

---

## 📚 相关文档

- `man 2 socket` - socket 类型
- `man 2 sendto` - DGRAM 发送
- `man 2 recvfrom` - DGRAM 接收
- `man 7 unix` - Unix Domain Socket
- `examples/README_SHM_DAEMON.md` - 使用示例

---

## 🎓 总结

这次修复解决了 **Socket 类型与系统调用不匹配**的根本问题：

1. **Socket 类型**：`SOCK_STREAM` → `SOCK_DGRAM`
2. **发送**：`send()` → `sendto()`
3. **接收**：`recv()` → `recvfrom()`
4. **地址管理**：保存发送者地址用于回复

修复后的代码**完全符合 SOCK_DGRAM 的语义**，适合守护进程查询等请求-响应场景。

---

**Date**: 2025-10-21
**Modified Files**:
- `zerocp_foundationLib/posix/ipc/source/unix_domainsocket.cpp`
- `zerocp_foundationLib/posix/ipc/include/unix_domainsocket.hpp`

