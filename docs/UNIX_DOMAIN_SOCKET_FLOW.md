# Unix Domain Socket 通信流程详解

## 🔄 完整流程图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                  Unix Domain Socket 进程间通信流程                       │
└─────────────────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════════
                           初始化阶段
═══════════════════════════════════════════════════════════════════════════

服务端                                              客户端
  │                                                   │
  │  1️⃣ socket(AF_UNIX, SOCK_STREAM, 0)              │
  ├────► 创建 socket 文件描述符                       │
  │      返回: fd = 3                                 │
  │                                                   │
  │  2️⃣ bind(fd, &addr, len)                          │
  ├────► 绑定到文件系统路径                           │
  │      addr.sun_path = "/tmp/my.sock"              │
  │      在文件系统中创建 socket 文件                  │
  │                                                   │
  │  3️⃣ listen(fd, backlog)                           │
  ├────► 开始监听连接请求                             │
  │      设置等待队列长度 = 10                         │
  │                                                   │
  ▼                                                   │
准备完成                                              │
等待连接...                                           │
  │                                                   │

═══════════════════════════════════════════════════════════════════════════
                          连接建立阶段
═══════════════════════════════════════════════════════════════════════════

  │                                                   │
  │                                      1️⃣ socket(AF_UNIX, SOCK_STREAM, 0)
  │                                      ├────► 创建客户端 socket
  │                                      │      返回: fd = 4
  │                                      │
  │                                      2️⃣ connect(fd, &addr, len)
  │  ◄────────────────连接请求──────────┤
  │                                      │
  4️⃣ accept(server_fd, NULL, NULL)        │
  ├────► 接受连接请求                    │
  │      返回: client_fd = 5             │
  │                                      │
  │  ─────────────────连接成功──────────►│
  │                                      │
  ▼                                      ▼
连接建立                                连接建立
client_fd = 5                          fd = 4

═══════════════════════════════════════════════════════════════════════════
                          数据传输阶段
═══════════════════════════════════════════════════════════════════════════

  │                                      │
  │  ◄──────────数据包 "Hello"──────────┤ 5️⃣ send(fd, "Hello", 5, 0)
  │                                      │
  5️⃣ recv(client_fd, buffer, 256, 0)     │
  ├────► 接收数据                        │
  │      buffer = "Hello"                │
  │      返回: 5 字节                     │
  │                                      │
  │  处理数据...                          │
  │  生成响应 "World"                     │
  │                                      │
  6️⃣ send(client_fd, "World", 5, 0)      │
  ├────► 发送响应                        │
  │  ──────────数据包 "World"──────────►│
  │                                      │
  │                                      6️⃣ recv(fd, buffer, 256, 0)
  │                                      ├────► 接收响应
  │                                      │      buffer = "World"
  │                                      │      返回: 5 字节
  │                                      │

═══════════════════════════════════════════════════════════════════════════
                          连接关闭阶段
═══════════════════════════════════════════════════════════════════════════

  │                                      │
  │                                      7️⃣ close(fd)
  │  ◄────────────关闭连接──────────────┤
  │                                      │
  7️⃣ close(client_fd)                    │
  ├────► 关闭客户端连接                  │
  │      释放 fd = 5                      │
  │                                      ▼
  │                                    连接关闭
  │
  8️⃣ close(server_fd)
  ├────► 关闭服务端 socket
  │      释放 fd = 3
  │
  9️⃣ unlink("/tmp/my.sock")
  ├────► 删除 socket 文件
  │
  ▼
清理完成
```

## 📊 Linux 内核层面的工作流程

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        Linux 内核处理流程                                │
└─────────────────────────────────────────────────────────────────────────┘

用户空间                   内核空间                    文件系统
   │                         │                           │
   │  socket()               │                           │
   ├─────────────────────►   │                           │
   │                      创建 socket 对象                │
   │                      分配内核缓冲区                  │
   │  ◄─────────────────────┤                           │
   │  返回 fd               │                           │
   │                         │                           │
   │  bind()                 │                           │
   ├─────────────────────►   │                           │
   │                      创建 VFS inode                  │
   │                         ├──────────────────────────►│
   │                         │                      创建 socket 文件
   │  ◄─────────────────────┤                           │
   │  绑定成功               │                           │
   │                         │                           │
   │  listen()               │                           │
   ├─────────────────────►   │                           │
   │                      设置 socket 状态为 LISTEN       │
   │                      初始化连接队列                  │
   │  ◄─────────────────────┤                           │
   │                         │                           │
   │  accept() [阻塞]        │                           │
   ├─────────────────────►   │                           │
   │                      等待连接队列...                │
   │                         │                           │

                        [客户端发起连接]

   │                         │                           │
   │                      检测到连接请求                 │
   │                      创建新的 socket                │
   │                      三次握手（内部）               │
   │  ◄─────────────────────┤                           │
   │  返回新 fd             │                           │
   │                         │                           │
   │  send()/recv()          │                           │
   ├─────────────────────►   │                           │
   │                      数据拷贝到内核缓冲区            │
   │                      ├──────────────┐               │
   │                      │  发送缓冲区   │               │
   │                      │  接收缓冲区   │               │
   │                      └──────────────┘               │
   │                      通过内核传输数据                │
   │  ◄─────────────────────┤                           │
   │                         │                           │
   │  close()                │                           │
   ├─────────────────────►   │                           │
   │                      释放 socket 资源                │
   │                      清理内核缓冲区                  │
   │                         │                           │
   │  unlink()               │                           │
   ├─────────────────────►   │                           │
   │                         ├──────────────────────────►│
   │                         │                      删除 socket 文件
   │  ◄─────────────────────┤                           │
   │                         │                           │
```

## 🔍 系统调用详解

### 1. socket() 系统调用

```c
int socket(int domain, int type, int protocol);
```

**参数**:
- `domain`: AF_UNIX (本地通信)
- `type`: SOCK_STREAM (可靠流) 或 SOCK_DGRAM (数据报)
- `protocol`: 0 (自动选择)

**内核操作**:
1. 分配 `struct socket` 对象
2. 分配发送和接收缓冲区
3. 初始化 socket 状态
4. 返回文件描述符

**返回值**: 成功返回 fd，失败返回 -1

---

### 2. bind() 系统调用

```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

**参数**:
- `sockfd`: socket 文件描述符
- `addr`: 指向 `struct sockaddr_un` 的指针
- `addrlen`: 地址结构大小

**内核操作**:
1. 验证路径有效性（长度 < 108）
2. 在文件系统中创建 socket 文件
3. 将 socket 与路径关联
4. 设置文件权限

**返回值**: 成功返回 0，失败返回 -1

---

### 3. listen() 系统调用

```c
int listen(int sockfd, int backlog);
```

**参数**:
- `sockfd`: socket 文件描述符
- `backlog`: 等待队列的最大长度

**内核操作**:
1. 将 socket 状态设置为 LISTEN
2. 初始化连接队列（已完成队列 + 未完成队列）
3. 准备接受连接请求

**返回值**: 成功返回 0，失败返回 -1

---

### 4. accept() 系统调用

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

**参数**:
- `sockfd`: 监听 socket 的文件描述符
- `addr`: 用于存储客户端地址（可以为 NULL）
- `addrlen`: 地址结构大小

**内核操作**:
1. 检查连接队列
2. 如果队列为空，阻塞等待
3. 从队列中取出一个连接
4. 创建新的 socket 对象
5. 返回新的文件描述符

**返回值**: 成功返回新连接的 fd，失败返回 -1

---

### 5. connect() 系统调用

```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

**参数**:
- `sockfd`: 客户端 socket 文件描述符
- `addr`: 服务端地址
- `addrlen`: 地址结构大小

**内核操作**:
1. 查找服务端 socket
2. 发送连接请求到服务端的连接队列
3. 等待服务端 accept
4. 建立连接

**返回值**: 成功返回 0，失败返回 -1

---

### 6. send() 系统调用

```c
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
```

**参数**:
- `sockfd`: socket 文件描述符
- `buf`: 要发送的数据
- `len`: 数据长度
- `flags`: 标志位（通常为 0）

**内核操作**:
1. 将数据从用户空间拷贝到内核发送缓冲区
2. 传输数据到对端的接收缓冲区
3. 如果缓冲区满，可能阻塞

**返回值**: 成功返回发送的字节数，失败返回 -1

---

### 7. recv() 系统调用

```c
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

**参数**:
- `sockfd`: socket 文件描述符
- `buf`: 接收缓冲区
- `len`: 缓冲区大小
- `flags`: 标志位（通常为 0）

**内核操作**:
1. 从内核接收缓冲区读取数据
2. 将数据拷贝到用户空间
3. 如果缓冲区空，可能阻塞

**返回值**: 成功返回接收的字节数，连接关闭返回 0，失败返回 -1

---

### 8. close() 系统调用

```c
int close(int fd);
```

**内核操作**:
1. 减少文件描述符引用计数
2. 如果引用计数为 0，释放资源
3. 清理内核缓冲区
4. 通知对端连接关闭

**返回值**: 成功返回 0，失败返回 -1

---

### 9. unlink() 系统调用

```c
int unlink(const char *pathname);
```

**内核操作**:
1. 从文件系统中删除指定文件
2. 如果没有进程打开该文件，立即删除
3. 否则等到最后一个进程关闭文件

**返回值**: 成功返回 0，失败返回 -1

## 📈 数据传输原理

### 内核缓冲区机制

```
┌─────────────────────────────────────────────────────────────┐
│                    数据传输示意图                            │
└─────────────────────────────────────────────────────────────┘

进程 A (发送方)            内核空间              进程 B (接收方)
     │                       │                        │
     │  send(data)           │                        │
     ├──────────────────►    │                        │
     │                  ┌────▼─────┐                 │
     │                  │  发送缓冲区 │                 │
     │                  │ [4K-64K]  │                 │
     │                  └────┬─────┘                 │
     │                       │                        │
     │                  内核传输                      │
     │                   (零拷贝)                     │
     │                       │                        │
     │                  ┌────▼─────┐                 │
     │                  │  接收缓冲区 │                 │
     │                  │ [4K-64K]  │                 │
     │                  └────┬─────┘                 │
     │                       │                        │
     │                       │  recv(buffer)          │
     │                       ├───────────────────►    │
     │                       │                        ▼
     │                       │                   buffer[]

特点:
✅ 数据只在内核空间传输
✅ 避免网络协议栈开销
✅ 高效的零拷贝传输
```

## 🔐 安全机制

### 文件权限控制

```bash
# socket 文件的权限示例
$ ls -l /tmp/*.sock
srwxr-xr-x 1 user group 0 Oct 20 10:30 /tmp/my.sock
│││││││││
││││││││└─ 其他用户: 可执行
│││││││└── 其他用户: 可读
││││││└─── 其他用户: 可写
│││││└──── 组: 可执行
││││└───── 组: 可读
│││└────── 组: 可写
││└─────── 所有者: 可执行
│└──────── 所有者: 可读
└───────── 所有者: 可写

s = socket 文件类型
```

### 权限设置

```cpp
// 仅所有者可访问 (0700)
.filePermissions(Perms::OwnerAll)

// 所有者读写 (0600)
.filePermissions(Perms::OwnerRead | Perms::OwnerWrite)

// 所有用户都可访问 (0777)
.filePermissions(Perms::All)
```

## 💡 最佳实践

### 1. 错误处理

```cpp
auto result = UnixDomainSocketBuilder()
    .path("/tmp/my.sock")
    .createServer();

if (!result)
{
    // 处理错误
    switch (result.error())
    {
        case UnixDomainSocketError::PATH_TOO_LONG:
            // 路径太长
            break;
        case UnixDomainSocketError::BIND_FAILED:
            // 绑定失败，可能权限不足或路径已被占用
            break;
        // ... 其他错误
    }
    return;
}
```

### 2. 资源清理

```cpp
{
    auto server = UnixDomainSocketBuilder()
        .path("/tmp/my.sock")
        .createServer();
    
    // ... 使用 server ...
    
}  // 离开作用域时自动清理（RAII）
   // - 关闭文件描述符
   // - 删除 socket 文件
```

### 3. 完整的数据传输

```cpp
// 完整发送函数
bool sendAll(UnixDomainSocket& socket, const void* data, size_t size)
{
    size_t totalSent = 0;
    const char* ptr = static_cast<const char*>(data);
    
    while (totalSent < size)
    {
        ssize_t sent = socket.send(ptr + totalSent, size - totalSent);
        if (sent <= 0)
        {
            return false;  // 发送失败
        }
        totalSent += sent;
    }
    
    return true;
}

// 完整接收函数
bool recvAll(UnixDomainSocket& socket, void* buffer, size_t size)
{
    size_t totalReceived = 0;
    char* ptr = static_cast<char*>(buffer);
    
    while (totalReceived < size)
    {
        ssize_t received = socket.receive(ptr + totalReceived, 
                                          size - totalReceived);
        if (received <= 0)
        {
            return false;  // 接收失败或连接关闭
        }
        totalReceived += received;
    }
    
    return true;
}
```

### 4. 超时处理

```cpp
#include <sys/socket.h>
#include <sys/time.h>

void setSocketTimeout(int fd, int seconds)
{
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    
    // 设置接收超时
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // 设置发送超时
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

// 使用
auto socket = builder.createClient().value();
setSocketTimeout(socket.getFileDescriptor(), 5);  // 5秒超时
```

---

**相关文档**:
- [详细使用指南](../test/posix/ipc/UNIX_DOMAIN_SOCKET_GUIDE.md)
- [API 文档](../zerocp_foundationLib/posix/ipc/include/unix_domainsocket.hpp)
- [测试代码](../test/posix/ipc/test_unix_domainsocket.cpp)

