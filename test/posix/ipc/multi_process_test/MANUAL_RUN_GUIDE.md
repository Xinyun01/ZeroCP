# 手动启动测试指南

本文档说明如何手动单独启动服务端和客户端进行测试。

## 📁 可执行文件位置

所有编译后的可执行文件位于：
```bash
/home/xinyun/Infrastructure/zero_copy_framework/test/posix/ipc/multi_process_test/build/
```

包含以下文件：
- `uds_server` - 服务端程序
- `uds_client1` - 客户端 1
- `uds_client2` - 客户端 2
- `uds_client3` - 客户端 3
- `uds_client4` - 客户端 4
- `uds_client5` - 客户端 5

---

## 🚀 方法 1：终端手动启动（推荐学习使用）

### 步骤 1：打开第一个终端，启动服务端

```bash
cd /home/xinyun/Infrastructure/zero_copy_framework/test/posix/ipc/multi_process_test/build
./uds_server
```

你会看到：
```
╔════════════════════════════════════════════════════════════╗
║           Multi-Process UDS Server Starting               ║
╚════════════════════════════════════════════════════════════╝

[SERVER] 🚀 Server Configuration:
  - Socket Path:       /tmp/uds_multi_process_server.sock
  - Expected Clients:  5
  - Messages/Client:   5
  - Total Messages:    25

[SERVER] ✅ Server socket created and bound
[SERVER] 🎧 Listening for incoming messages...
```

**保持此终端打开，服务端会持续监听。**

---

### 步骤 2：打开新终端，启动客户端

**客户端 1：**
```bash
cd /home/xinyun/Infrastructure/zero_copy_framework/test/posix/ipc/multi_process_test/build
./uds_client1
```

**客户端 2：**
```bash
cd /home/xinyun/Infrastructure/zero_copy_framework/test/posix/ipc/multi_process_test/build
./uds_client2
```

**客户端 3-5 同理：**
```bash
./uds_client3
./uds_client4
./uds_client5
```

你可以：
- ✅ 在同一个终端依次运行多个客户端
- ✅ 在不同终端同时运行多个客户端
- ✅ 随时启动任意客户端，服务端会响应

---

### 步骤 3：观察通信过程

**客户端输出示例：**
```
[CLIENT-1] 📤 [1/5] Sending: "Client-1 Message-1"
[CLIENT-1] ✅ Message sent successfully
[CLIENT-1] ⏳ Waiting for server response...
[CLIENT-1] 📨 [1/5] Received: "ACK: Client-1 Message-1" ✅
[CLIENT-1] ✅ Response verified from server: /tmp/uds_multi_process_server.sock
```

**服务端输出示例：**
```
[SERVER] 📨 [Message 1] Received from: /tmp/uds_client_1.sock
[SERVER]    Content: "Client-1 Message-1"
[SERVER] 📤 [Message 1] Replied to: /tmp/uds_client_1.sock
[SERVER]    Response: "ACK: Client-1 Message-1" ✅
```

---

### 步骤 4：停止服务端

在服务端终端按 `Ctrl + C` 或在另一个终端执行：
```bash
pkill uds_server
```

---

## 🚀 方法 2：后台启动服务端

如果你想让服务端在后台运行：

```bash
cd /home/xinyun/Infrastructure/zero_copy_framework/test/posix/ipc/multi_process_test/build

# 启动服务端（后台运行）
./uds_server > server.log 2>&1 &

# 记录进程ID
SERVER_PID=$!
echo "服务端 PID: $SERVER_PID"

# 等待服务端启动
sleep 1

# 运行客户端
./uds_client1
./uds_client2
./uds_client3

# 查看服务端日志
tail -f server.log

# 停止服务端
kill $SERVER_PID
```

---

## 🚀 方法 3：一键启动脚本

创建快捷启动脚本 `quick_start.sh`：

```bash
#!/bin/bash

BUILD_DIR="/home/xinyun/Infrastructure/zero_copy_framework/test/posix/ipc/multi_process_test/build"
cd $BUILD_DIR

# 启动服务端
./uds_server &
SERVER_PID=$!
echo "✅ 服务端已启动 (PID: $SERVER_PID)"

# 等待服务端启动
sleep 1

# 启动指定的客户端（可传入参数）
if [ $# -eq 0 ]; then
    echo "用法: $0 <client_number>"
    echo "示例: $0 1  (启动客户端1)"
    echo "示例: $0 2  (启动客户端2)"
    kill $SERVER_PID
    exit 1
fi

CLIENT_NUM=$1
echo "▶️  启动客户端 $CLIENT_NUM..."
./uds_client${CLIENT_NUM}

# 停止服务端
kill $SERVER_PID 2>/dev/null
echo "✅ 服务端已停止"
```

使用方法：
```bash
chmod +x quick_start.sh
./quick_start.sh 1  # 启动服务端 + 客户端1
./quick_start.sh 2  # 启动服务端 + 客户端2
```

---

## 🔍 查看运行状态

### 查看进程
```bash
ps aux | grep uds_
```

### 查看 Socket 文件
```bash
ls -l /tmp/uds_*.sock
```

输出示例：
```
srwxrwxr-x 1 xinyun xinyun 0 Oct 21 20:37 /tmp/uds_multi_process_server.sock
srwxrwxr-x 1 xinyun xinyun 0 Oct 21 20:38 /tmp/uds_client_1.sock
```

### 清理 Socket 文件
```bash
rm -f /tmp/uds_*.sock
```

---

## 📊 验证测试成功

客户端成功的标志：
```
========================================
[CLIENT-X] 📊 Client Statistics
========================================
Successful Exchanges: 5 / 5        ← 全部成功
Failed Exchanges:     0 / 5        ← 无失败
Success Rate:         100%         ← 100% 成功率
========================================
[CLIENT-X] ✅ All exchanges completed successfully!
========================================
```

---

## 🛠️ 常见问题

### Q1: 客户端无法连接服务端？
**A:** 确保服务端已启动：
```bash
ps aux | grep uds_server
```

### Q2: 提示 "Address already in use"？
**A:** 清理旧的 socket 文件：
```bash
rm -f /tmp/uds_multi_process_server.sock
```

### Q3: 如何同时运行多个客户端？
**A:** 打开多个终端窗口，或使用 `&` 后台运行：
```bash
./uds_client1 &
./uds_client2 &
./uds_client3 &
wait  # 等待所有客户端完成
```

### Q4: 服务端如何优雅退出？
**A:** 在服务端终端按 `Ctrl + C`，或发送 SIGINT 信号：
```bash
kill -SIGINT <server_pid>
```

---

## 📝 测试场景示例

### 场景 1：单客户端测试
```bash
# Terminal 1
./uds_server

# Terminal 2
./uds_client1
```

### 场景 2：多客户端顺序测试
```bash
# Terminal 1
./uds_server

# Terminal 2
./uds_client1
./uds_client2
./uds_client3
./uds_client4
./uds_client5
```

### 场景 3：多客户端并发测试
```bash
# Terminal 1
./uds_server

# Terminal 2-6（分别打开5个终端）
./uds_client1  # Terminal 2
./uds_client2  # Terminal 3
./uds_client3  # Terminal 4
./uds_client4  # Terminal 5
./uds_client5  # Terminal 6
```

---

## 🎯 总结

| 方法 | 适用场景 | 优点 |
|------|---------|------|
| **多终端启动** | 学习调试 | 可实时看到服务端和客户端日志 |
| **后台启动** | 快速测试 | 一个终端完成所有操作 |
| **自动化脚本** | 完整测试 | 使用 `./run_test.sh` 自动化测试 |

**推荐顺序：**
1. 先用多终端手动启动，理解通信流程
2. 再用后台启动进行快速验证
3. 最后用 `run_test.sh` 进行完整自动化测试

享受测试！🎉


