# SharedChunk 跨进程传输指南

## 问题背景

在跨进程零拷贝传输中，一个关键问题是：**如何在源进程释放 SharedChunk 后，确保目标进程仍能安全访问数据？**

### 错误的方式

```cpp
// ❌ 错误：源进程
{
    SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
    // 写入数据...
    
    // 发送 chunkIndex 到目标进程
    sendToOtherProcess(chunk.getChunkManagerIndex());
    
} // chunk 析构 -> refCount 减到 0 -> 资源被释放！
  // 目标进程还没来得及接收，数据已经丢失

// ❌ 错误：目标进程
uint32_t index = receiveFromOtherProcess();
SharedChunk chunk = SharedChunk::fromIndex(index, poolMgr);
// 此时可能访问已释放的内存！
```

**问题：** 源进程的 SharedChunk 析构时会减少引用计数，如果目标进程还没有增加引用计数，资源就会被释放。

---

## 正确的方式

### 核心原则

跨进程传输时，需要在传输前 **显式增加引用计数**，确保即使源进程析构，目标进程仍能安全访问。

### API 设计

#### 1. `prepareForTransfer()` - 准备传输

```cpp
uint32_t SharedChunk::prepareForTransfer() noexcept;
```

**作用：**
- 增加引用计数（为目标进程预留引用）
- 返回 ChunkManagerIndex（可通过 IPC 传输）

**时机：** 在发送 IPC 消息之前调用

#### 2. `fromIndex()` - 重建 SharedChunk

```cpp
static SharedChunk SharedChunk::fromIndex(uint32_t index, MemPoolManager* memPoolManager) noexcept;
```

**作用：**
- 通过索引重建 ChunkManager 指针
- 创建 SharedChunk（不增加引用计数，因为发送端已经增加过）

**时机：** 接收端收到 IPC 消息后调用

---

## 完整示例

### 发送端（进程 A）

```cpp
#include "shared_chunk.hpp"
#include "mempool_manager.hpp"

void senderProcess()
{
    MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();
    
    // 1. 分配 chunk
    SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
    
    // 2. 写入数据
    void* data = chunk.getData();
    memcpy(data, "Hello from sender", 17);
    
    // 3. 准备传输（关键步骤！）
    uint32_t chunkIndex = chunk.prepareForTransfer();
    // 此时 refCount = 2（当前进程 1 + 目标进程预留 1）
    
    // 4. 通过 IPC 发送索引
    sendMessageQueue(chunkIndex);  // 例如：POSIX 消息队列
    
    // 5. chunk 析构（refCount 减到 1，不会真正释放）
    // 资源安全，等待目标进程接收
}
```

### 接收端（进程 B）

```cpp
#include "shared_chunk.hpp"
#include "mempool_manager.hpp"

void receiverProcess()
{
    MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();
    
    // 1. 从 IPC 接收索引
    uint32_t chunkIndex = receiveMessageQueue();
    
    // 2. 重建 SharedChunk（不增加引用计数）
    SharedChunk chunk = SharedChunk::fromIndex(chunkIndex, poolMgr);
    // 此时 refCount = 1（发送端已释放）
    
    // 3. 读取数据
    void* data = chunk.getData();
    std::cout << static_cast<char*>(data) << std::endl;
    
    // 4. chunk 析构（refCount 减到 0，资源被释放）
    // 数据处理完毕，资源正确回收
}
```

---

## 引用计数变化流程

| 时间点 | 操作 | refCount | 说明 |
|--------|------|----------|------|
| T1 | 发送端：分配 chunk | 1 | 初始引用 |
| T2 | 发送端：prepareForTransfer() | 2 | 为目标进程预留 |
| T3 | 发送端：发送 IPC 消息 | 2 | 索引传输中 |
| T4 | 发送端：SharedChunk 析构 | 1 | 减少但不释放 |
| T5 | 接收端：fromIndex() 重建 | 1 | 接管所有权 |
| T6 | 接收端：处理数据 | 1 | 使用中 |
| T7 | 接收端：SharedChunk 析构 | 0 | 释放资源 |

---

## 单元测试

参见 `test/chunktest/test_shared_chunk.cpp` 中的 `testCrossProcessTransfer()` 函数：

```cpp
void testCrossProcessTransfer()
{
    // 1. 模拟发送端
    SharedChunk senderChunk(poolMgr->getChunk(256), poolMgr);
    uint32_t index = senderChunk.prepareForTransfer();
    senderChunk.reset();  // 模拟析构
    
    // 2. 模拟接收端
    SharedChunk receiverChunk = SharedChunk::fromIndex(index, poolMgr);
    // 验证数据完整性
    receiverChunk.reset();  // 资源释放
}
```

运行测试：
```bash
cd test/chunktest
./build.sh
./test_shared_chunk
```

---

## 常见错误

### 错误 1：忘记调用 prepareForTransfer()

```cpp
// ❌ 错误
SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
sendMessageQueue(chunk.getChunkManagerIndex());  // 直接发送，没有增加引用计数
// chunk 析构 -> 资源立即释放
```

**后果：** 目标进程访问已释放的内存，导致数据损坏或段错误。

**修复：**
```cpp
// ✓ 正确
SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
uint32_t index = chunk.prepareForTransfer();  // 先增加引用计数
sendMessageQueue(index);
```

### 错误 2：接收端重复增加引用计数

```cpp
// ❌ 错误
uint32_t index = receiveMessageQueue();
SharedChunk temp(poolMgr->getChunkManagerByIndex(index), poolMgr);
SharedChunk chunk = temp;  // 拷贝构造会再次增加引用计数
// 现在 refCount = 2，即使两个 SharedChunk 都析构，引用计数也不会降到 0
```

**后果：** 内存泄漏，chunk 永远不会被释放。

**修复：**
```cpp
// ✓ 正确
uint32_t index = receiveMessageQueue();
SharedChunk chunk = SharedChunk::fromIndex(index, poolMgr);  // 直接使用静态方法
```

### 错误 3：发送失败后没有回滚引用计数

```cpp
// ❌ 错误
SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
uint32_t index = chunk.prepareForTransfer();  // refCount = 2
if (!sendMessageQueue(index))  // 发送失败
{
    // 没有减少引用计数，导致内存泄漏
    return;
}
```

**后果：** 引用计数永远不会降到 0，内存泄漏。

**修复：**
```cpp
// ✓ 正确
SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
uint32_t index = chunk.prepareForTransfer();
if (!sendMessageQueue(index))
{
    // 发送失败，手动释放预留的引用
    SharedChunk rollback = SharedChunk::fromIndex(index, poolMgr);
    rollback.reset();  // 减少引用计数
    return;
}
```

---

## 高级话题

### 多播（1 发送端 -> N 接收端）

如果需要将同一个 chunk 发送给多个进程：

```cpp
// 发送端
SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
uint32_t index = chunk.getChunkManagerIndex();

// 为 N 个接收端分别增加引用计数
for (int i = 0; i < N; ++i)
{
    chunk.prepareForTransfer();  // 每次调用都增加 refCount
    sendToReceiver(i, index);
}

// chunk 析构后，refCount = N（等待 N 个接收端处理）
```

### 超时处理

如果接收端崩溃或超时未接收，需要有机制回收资源：

```cpp
// 实现思路（伪代码）
struct PendingTransfer
{
    uint32_t chunkIndex;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
};

std::vector<PendingTransfer> pendingTransfers;

// 定期检查超时
void cleanupTimedOutTransfers()
{
    auto now = std::chrono::steady_clock::now();
    for (auto it = pendingTransfers.begin(); it != pendingTransfers.end(); )
    {
        if (now - it->timestamp > std::chrono::seconds(30))
        {
            // 超时，回收引用计数
            SharedChunk chunk = SharedChunk::fromIndex(it->chunkIndex, poolMgr);
            chunk.reset();
            it = pendingTransfers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
```

---

## 总结

| 操作 | API | 引用计数变化 | 使用场景 |
|------|-----|--------------|----------|
| 分配 chunk | `getChunk()` + 构造函数 | +1 | 初始分配 |
| 准备传输 | `prepareForTransfer()` | +1 | 跨进程发送前 |
| 重建 chunk | `fromIndex()` | 0 | 跨进程接收端 |
| 拷贝 | 拷贝构造/赋值 | +1 | 进程内共享 |
| 移动 | 移动构造/赋值 | 0 | 转移所有权 |
| 释放 | 析构/reset() | -1 | 不再使用 |

**关键记忆点：**
1. **跨进程传输 = prepareForTransfer() + fromIndex()**
2. **发送端调用 prepareForTransfer()，接收端调用 fromIndex()**
3. **只传输索引（uint32_t），不传输指针**
4. **失败时记得回滚引用计数**

---

## 参考资料

- `shared_chunk.hpp` - SharedChunk 类定义
- `shared_chunk.cpp` - 实现细节
- `test_shared_chunk.cpp` - 单元测试示例
- `chunk_manager.hpp` - ChunkManager 结构定义
- `mempool_manager.hpp` - MemPoolManager API

