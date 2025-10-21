# 守护进程与共享内存地址查询示例

使用 **Unix Domain Socket (SOCK_DGRAM)** 实现守护进程向多个客户端进程提供共享内存地址查询服务。

## 🎯 使用场景

多个进程需要访问同一块共享内存，但不想硬编码共享内存路径。通过守护进程集中管理和分发共享内存地址：

```
┌─────────────┐
│  守护进程    │  管理共享内存信息
│ (Server)    │  /dev/shm/xxx_shm
└──────┬──────┘
       │ Unix Domain Socket (DGRAM)
       │
       ├──────────┬──────────┬──────────┐
       │          │          │          │
   ┌───▼───┐  ┌──▼────┐  ┌──▼────┐  ┌──▼────┐
   │进程 A │  │进程 B │  │进程 C │  │进程 D │
   └───────┘  └───────┘  └───────┘  └───────┘
       │          │          │          │
       └──────────┴──────────┴──────────┘
                  │
              共享内存
```

## 📁 文件说明

### 1. `daemon_shm_server.cpp`
守护进程（服务端），功能：
- 绑定 Unix Domain Socket
- 循环接收客户端查询请求
- 返回共享内存路径和大小

### 2. `client_shm_query.cpp`
客户端进程，功能：
- 连接守护进程
- 查询共享内存路径
- 展示如何使用返回的地址

## 🚀 快速开始

### 编译

```bash
cd /home/xinyun/Infrastructure/zero_copy_framework

# 编译守护进程
g++ -std=c++20 \
    -I./zerocp_foundationLib/posix/ipc/include \
    -I./zerocp_foundationLib/report/include \
    -I./zerocp_foundationLib/container/include \
    -I./zerocp_foundationLib/design \
    -I./zerocp_foundationLib/posix/call/include \
    examples/daemon_shm_server.cpp \
    zerocp_foundationLib/posix/ipc/source/unix_domainsocket.cpp \
    -o examples/daemon_shm_server

# 编译客户端
g++ -std=c++20 \
    -I./zerocp_foundationLib/posix/ipc/include \
    -I./zerocp_foundationLib/report/include \
    -I./zerocp_foundationLib/container/include \
    -I./zerocp_foundationLib/design \
    -I./zerocp_foundationLib/posix/call/include \
    examples/client_shm_query.cpp \
    zerocp_foundationLib/posix/ipc/source/unix_domainsocket.cpp \
    -o examples/client_shm_query
```

### 运行

**终端 1 - 启动守护进程：**
```bash
./examples/daemon_shm_server
```

**终端 2, 3, 4... - 运行多个客户端：**
```bash
./examples/client_shm_query
```

## 📋 协议说明

### 支持的命令

| 请求命令 | 响应 | 说明 |
|---------|------|------|
| `GET_SHM_PATH` | `/dev/shm/zero_copy_framework_shm` | 获取共享内存路径 |
| `GET_SHM_SIZE` | `4096` | 获取共享内存大小（字节）|
| `PING` | `PONG` | 健康检查 |
| 其他 | `ERROR: Unknown command` | 未知命令 |

### 通信模式

**SOCK_DGRAM (数据报模式)**：
- ✅ **无连接**：客户端直接发送，无需 listen/accept
- ✅ **独立消息**：每个请求-响应独立
- ✅ **简单**：适合查询类场景
- ✅ **多客户端**：天然支持多客户端同时查询

**通信流程**：
```
Client                Server
  |                     |
  |--"GET_SHM_PATH"---->| (无需建立连接)
  |                     |
  |<--"/dev/shm/xxx"----|
  |                     |
```

## 💡 实际应用示例

### 完整流程

```cpp
// ========== 守护进程 (daemon_shm_server.cpp) ==========
int main() {
    // 1. 创建/管理共享内存
    const char* shmPath = "/dev/shm/zero_copy_framework_shm";
    
    // 2. 启动服务端监听查询
    auto server = UnixDomainSocketBuilder()
        .name("/tmp/shm_daemon.sock")
        .channelSide(SERVER)
        .create();
    
    // 3. 响应客户端查询
    while (running) {
        std::string request;
        server.receive(request);
        
        if (request == "GET_SHM_PATH") {
            server.send(shmPath);
        }
    }
}

// ========== 客户端进程 A, B, C... (client_shm_query.cpp) ==========
int main() {
    // 1. 查询守护进程获取共享内存地址
    auto client = UnixDomainSocketBuilder()
        .name("/tmp/shm_daemon.sock")
        .channelSide(CLIENT)
        .create();
    
    client.send("GET_SHM_PATH");
    
    std::string shmPath;
    client.receive(shmPath);
    // 得到: "/dev/shm/zero_copy_framework_shm"
    
    // 2. 使用获取到的路径打开共享内存
    int shmFd = shm_open(shmPath.c_str(), O_RDWR, 0666);
    ftruncate(shmFd, 4096);
    
    void* addr = mmap(NULL, 4096, 
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED, shmFd, 0);
    
    // 3. 使用共享内存进行零拷贝通信
    // 写入数据
    memcpy(addr, myData, dataSize);
    
    // 读取数据（其他进程写入的）
    processData(addr);
    
    // 4. 清理
    munmap(addr, 4096);
    close(shmFd);
}
```

## 🔄 SOCK_DGRAM vs SOCK_STREAM

### 为什么选择 SOCK_DGRAM？

| 特性 | SOCK_DGRAM (数据报) | SOCK_STREAM (流) |
|------|-------------------|------------------|
| 连接 | ❌ 无连接 | ✅ 面向连接 |
| 复杂度 | ✅ 简单 | ⚠️ 需要 listen/accept |
| 消息边界 | ✅ 保留 | ❌ 字节流，需要自己处理 |
| 可靠性 | ✅ Unix 域可靠 | ✅ 可靠 |
| 适用场景 | ✅ **查询/请求-响应** | ⚠️ 长连接/会话 |
| 多客户端 | ✅ 天然支持 | ⚠️ 需要 accept 循环 |

**结论**：对于"查询共享内存地址"这种**简单请求-响应**场景，**SOCK_DGRAM 是最佳选择**！

## 🎓 扩展应用

### 1. 动态共享内存管理

守护进程可以动态创建和管理多个共享内存区域：

```cpp
// 守护进程支持多个共享内存区域
std::map<std::string, ShmInfo> shmRegistry;

// 客户端请求特定用途的共享内存
client.send("GET_SHM_PATH:video_buffer");
// 响应: "/dev/shm/video_buffer_shm"

client.send("GET_SHM_PATH:audio_buffer");
// 响应: "/dev/shm/audio_buffer_shm"
```

### 2. 资源配额管理

```cpp
// 守护进程记录每个进程的使用情况
client.send("REGISTER:process_id=1234");
client.send("GET_SHM_PATH");  // 分配共享内存
client.send("RELEASE");        // 释放共享内存
```

### 3. 权限控制

```cpp
// 守护进程验证客户端权限
client.send("AUTH:token=xxxxx");
client.send("GET_SHM_PATH");  // 只有认证的客户端才能访问
```

## 🔍 调试技巧

### 1. 查看 Unix Socket
```bash
ls -la /tmp/*.sock
```

### 2. 手动测试（使用 socat）
```bash
# 发送查询
echo "GET_SHM_PATH" | socat - UNIX-CONNECT:/tmp/shm_daemon.sock
```

### 3. 监控系统调用
```bash
strace -e trace=socket,bind,sendto,recvfrom ./daemon_shm_server
```

### 4. 查看共享内存
```bash
ls -lh /dev/shm/
```

## ⚡ 性能特点

- **延迟**: 极低（本地 Unix Socket + 无连接建立开销）
- **吞吐量**: 高（数据报模式，无需等待连接）
- **并发**: 优秀（服务端可以同时处理多个客户端请求）
- **资源**: 低（无需为每个客户端维护连接状态）

## ❓ 常见问题

### Q: 多个客户端同时查询会冲突吗？
**A**: 不会。SOCK_DGRAM 模式下，每个数据报独立处理，守护进程会自动区分不同客户端。

### Q: 消息大小限制？
**A**: 当前限制为 `maxMsgSize`（默认 1024 字节），足够传输路径字符串。

### Q: 如果守护进程崩溃了怎么办？
**A**: 客户端的 `receive()` 会超时或失败。建议：
1. 客户端检测失败后重试
2. 使用 systemd 等工具自动重启守护进程

### Q: 能不能缓存共享内存地址，不每次都查询？
**A**: 可以！通常做法：
```cpp
// 启动时查询一次
static std::string shmPath = queryShmPath();

// 后续直接使用
useShmPath(shmPath);
```

## 📚 相关资源

- `man 7 unix` - Unix Domain Socket
- `man shm_open` - POSIX 共享内存
- `man mmap` - 内存映射

## 🛠️ 下一步

现在你可以：
1. ✅ 运行示例代码体验
2. ✅ 修改守护进程添加更多命令
3. ✅ 实现真正的共享内存创建和管理
4. ✅ 添加认证、日志、监控等功能

Happy coding! 🚀

