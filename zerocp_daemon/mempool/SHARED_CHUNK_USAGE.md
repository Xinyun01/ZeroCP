# SharedChunk 使用指南

## 概述

`SharedChunk` 是 `ChunkManager` 的 RAII 包装器，用于在多进程环境下自动管理共享内存 chunk 的引用计数。

## 设计原理

### 引用计数管理

类似于 `std::shared_ptr`，`SharedChunk` 通过原子引用计数来管理共享内存中的资源：

- **构造时**：接管 `ChunkManager`（不增加引用计数，假设已经是 1）
- **拷贝时**：增加引用计数（多个进程/对象共享同一块内存）
- **移动时**：转移所有权（不增加引用计数）
- **析构时**：减少引用计数，当引用计数为 0 时释放资源

### 多进程场景

```
进程 A                          共享内存                       进程 B
--------                      ------------                    --------
getChunk()              ChunkManager (refCount=1)
  |                              |
SharedChunk chunk1              |
  |                              |
发送 ChunkManager* ──────────────┼─────────────> 接收指针
  |                              |                    |
  |                     refCount=1 (还是1!)      SharedChunk chunk2 (拷贝构造)
  |                              |                    |
  |                     refCount=2 (增加到2)          |
  |                              |                    |
chunk1 析构 ──────────>  refCount=1 (减少到1)          |
  |                              |                    |
  |                              | <─────────── chunk2 析构
  |                     refCount=0 (减少到0)
  |                              |
  |                     释放资源，归还内存池
```

## 使用方法

### 1. 基本用法

```cpp
#include "zerocp_daemon/mempool/shared_chunk.hpp"
#include "zerocp_daemon/memory/include/mempool_manager.hpp"

using namespace ZeroCP::Memory;

// 获取内存池管理器
MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();

// 分配 chunk（返回的 ChunkManager 已经有 refCount=1）
ChunkManager* rawChunk = poolMgr->getChunk(1024);

// 使用 SharedChunk 包装（接管所有权，不增加引用计数）
SharedChunk chunk(rawChunk, poolMgr);

// 访问数据
void* data = chunk.getData();
uint64_t size = chunk.getSize();

// 写入数据
memcpy(data, "Hello", 5);

// 自动释放：chunk 析构时会减少引用计数
// 如果 refCount 变为 0，会自动归还到内存池
```

### 2. 跨进程传递（发送端）

```cpp
// 进程 A：发送端
SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);

// 写入数据
void* data = chunk.getData();
memcpy(data, "Message from A", 14);

// 跨进程传递：只发送 ChunkManager 指针的相对偏移
// 注意：不能直接发送指针！需要转换为相对偏移
ChunkManager* rawPtr = chunk.get();
// 发送 rawPtr 到进程 B（通过某种 IPC 机制）
// ...

// chunk 仍然持有引用，refCount 还是 1
```

### 3. 跨进程传递（接收端）

```cpp
// 进程 B：接收端
// 接收到 ChunkManager* 指针（已经映射到本进程地址空间）
ChunkManager* receivedPtr = ...; // 从 IPC 接收

// 创建 SharedChunk 并增加引用计数
MemPoolManager* poolMgr = MemPoolManager::getInstanceIfInitialized();

// 方法 1：使用拷贝构造（推荐）
SharedChunk tempChunk(receivedPtr, poolMgr);
SharedChunk chunk = tempChunk; // 拷贝构造，refCount 增加到 2

// 方法 2：手动增加引用计数后构造
receivedPtr->m_refCount.fetch_add(1, std::memory_order_acq_rel);
SharedChunk chunk(receivedPtr, poolMgr);

// 读取数据
void* data = chunk.getData();
std::cout << static_cast<char*>(data) << std::endl;

// 自动释放：chunk 析构时减少引用计数
```

### 4. 拷贝和移动语义

```cpp
SharedChunk chunk1(poolMgr->getChunk(1024), poolMgr);
// chunk1 持有 ChunkManager，refCount = 1

// 拷贝：增加引用计数
SharedChunk chunk2 = chunk1; // refCount = 2
SharedChunk chunk3(chunk1);  // refCount = 3

// 移动：转移所有权，不增加引用计数
SharedChunk chunk4 = std::move(chunk1); // refCount 还是 3
// chunk1 现在是空的

// 检查状态
if (chunk1) {
    // 这个分支不会执行，因为 chunk1 已经被移动
}

if (chunk4) {
    // chunk4 现在持有原来 chunk1 的资源
}

// 析构顺序：
// chunk4 析构 -> refCount = 2
// chunk3 析构 -> refCount = 1  
// chunk2 析构 -> refCount = 0 -> 释放资源
```

### 5. 手动管理

```cpp
SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);

// 查询引用计数
uint64_t count = chunk.useCount(); // 返回 1

// 检查是否有效
if (chunk.isValid()) {
    // 使用 chunk
}

// 手动释放（不等析构）
chunk.reset(); // 减少引用计数，可能释放资源

// 替换为新的 chunk
ChunkManager* newChunk = poolMgr->getChunk(2048);
chunk.reset(newChunk, poolMgr);
```

## 关键设计决策

### 为什么构造函数不增加引用计数？

```cpp
// 这是推荐的用法：
SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
```

因为 `MemPoolManager::getChunk()` 返回的 `ChunkManager` 已经有 `refCount=1`。
如果构造函数再增加一次，会导致 `refCount=2`，这样最后一个 `SharedChunk` 析构后，
引用计数只能减到 1，资源永远不会被释放（内存泄漏）！

### 为什么拷贝构造要增加引用计数？

```cpp
SharedChunk chunk1(rawChunk, poolMgr); // refCount = 1
SharedChunk chunk2 = chunk1;           // refCount = 2
```

拷贝表示创建了一个新的所有者，需要增加引用计数。当 `chunk1` 和 `chunk2` 都析构后，
`refCount` 才会减到 0，资源才会被释放。

### 为什么移动构造不增加引用计数？

```cpp
SharedChunk chunk1(rawChunk, poolMgr); // refCount = 1
SharedChunk chunk2 = std::move(chunk1); // refCount 还是 1
```

移动表示所有权转移，不是创建新的所有者。`chunk1` 被清空，`chunk2` 接管资源，
总的所有者数量没变，所以引用计数不变。

## 多进程使用注意事项

### ✅ 正确的做法

1. **发送端**：保持 `SharedChunk` 活着直到确认接收端已创建自己的 `SharedChunk`
2. **接收端**：收到指针后立即创建 `SharedChunk` 或手动增加引用计数
3. **传递指针**：使用相对偏移或 `RelativePointer`，不要直接传递虚拟地址

### ❌ 错误的做法

```cpp
// 错误 1：发送端过早释放
{
    SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
    sendPointer(chunk.get());
    // chunk 析构 -> refCount = 0 -> 资源被释放！
    // 接收端收到的是悬空指针！
}

// 错误 2：接收端不增加引用计数
ChunkManager* ptr = receivePointer();
// 直接使用 ptr，没有创建 SharedChunk
// 发送端的 SharedChunk 析构后，ptr 变成悬空指针

// 错误 3：重复接管所有权
ChunkManager* ptr = receivePointer();
SharedChunk chunk1(ptr, poolMgr); // 不增加 refCount
SharedChunk chunk2(ptr, poolMgr); // 还是不增加 refCount
// chunk1 析构 -> refCount = 0 -> 释放资源
// chunk2 析构 -> double-free！
```

### ✅ 正确的跨进程流程

```cpp
// 进程 A：发送端
void sender() {
    SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
    memcpy(chunk.getData(), "data", 4);
    
    // 发送指针
    sendPointer(chunk.get());
    
    // 等待接收端确认（可选，取决于你的同步机制）
    waitForAck();
    
    // chunk 析构 -> refCount -= 1
}

// 进程 B：接收端
void receiver() {
    ChunkManager* ptr = receivePointer();
    
    // 立即创建 SharedChunk 并增加引用计数
    SharedChunk temp(ptr, poolMgr);
    SharedChunk chunk = temp; // 拷贝构造，refCount += 1
    
    // 或者使用这种方式：
    // ptr->m_refCount.fetch_add(1, std::memory_order_acq_rel);
    // SharedChunk chunk(ptr, poolMgr);
    
    // 发送确认
    sendAck();
    
    // 使用数据
    std::cout << static_cast<char*>(chunk.getData()) << std::endl;
    
    // chunk 析构 -> refCount -= 1
}
```

## 调试技巧

### 打印引用计数

```cpp
SharedChunk chunk(poolMgr->getChunk(1024), poolMgr);
std::cout << "RefCount: " << chunk.useCount() << std::endl;

SharedChunk chunk2 = chunk;
std::cout << "RefCount after copy: " << chunk.useCount() << std::endl;
```

### 检查资源泄漏

```cpp
// 程序结束前检查内存池状态
poolMgr->printAllPoolStats();

// 如果有泄漏，会看到 "used chunks" 不为 0
```

## 与 MemPoolManager 的集成

### 修改 getChunk 接口（可选）

为了更方便使用，可以添加一个返回 `SharedChunk` 的方法：

```cpp
// 在 mempool_manager.hpp 中添加：
class MemPoolManager {
public:
    // 原有接口：返回裸指针
    ChunkManager* getChunk(uint64_t size) noexcept;
    
    // 新接口：返回 SharedChunk（更安全）
    SharedChunk allocateChunk(uint64_t size) noexcept {
        ChunkManager* rawChunk = getChunk(size);
        return SharedChunk(rawChunk, this);
    }
};

// 使用：
SharedChunk chunk = poolMgr->allocateChunk(1024);
```

## 性能考虑

- **原子操作开销**：每次拷贝/析构都涉及原子操作，但开销很小（通常几纳秒）
- **移动语义**：使用移动语义可以避免不必要的原子操作
- **避免不必要的拷贝**：尽量使用引用传递 `const SharedChunk&`

```cpp
// 好：按引用传递，不增加引用计数
void processChunk(const SharedChunk& chunk) {
    // 使用 chunk
}

// 差：按值传递，会拷贝并增加引用计数
void processChunk(SharedChunk chunk) {
    // 使用 chunk
}
```

## 总结

`SharedChunk` 提供了：
✅ 自动的引用计数管理
✅ RAII 保证资源不泄漏
✅ 跨进程共享的安全性
✅ 类似 `std::shared_ptr` 的直观语义
✅ 零拷贝的高性能（只管理引用，不拷贝数据）

遵循本指南的最佳实践，可以在多进程环境下安全、高效地管理共享内存！

