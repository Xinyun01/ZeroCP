# 共享内存基地址设计说明

## 设计原则：进程本地变量 vs 相对偏移

### 问题描述

在多进程共享内存架构中，每个进程映射同一个共享内存段时，**虚拟地址可能不同**。例如：

```
进程 A: 管理区映射到 0x7f0000000000, 数据区映射到 0x7f1000000000
进程 B: 管理区映射到 0x7e0000000000, 数据区映射到 0x7e1000000000
```

这意味着：
1. **不能在共享内存中存储绝对指针**（因为每个进程的地址不同）
2. **必须使用相对偏移量**来引用共享内存中的数据

### 解决方案

我们的架构采用了以下分层设计：

#### 1. 共享内存中的数据结构（使用相对偏移）

在共享内存中的所有数据结构都使用 `RelativePointer<T>` 来引用其他共享内存数据：

```cpp
class MemPool
{
private:
    RelativePointer<void> m_rawmemory;  // ✅ 使用相对指针
    // ...
};
```

`RelativePointer` 存储的是**相对于共享内存基地址的偏移量**，而不是绝对地址。

#### 2. 进程本地变量（记录映射地址）

每个进程都需要知道共享内存在**本进程**中映射到了哪里，所以我们使用**进程本地**的静态变量：

```cpp
class MemPoolManager
{
private:
    // ==================== 静态成员（进程本地） ====================
    
    static MemPoolManager* s_instance;              ///< 单例实例指针（指向管理区共享内存中的对象）
    static void* s_managementBaseAddress;           ///< 管理区共享内存基地址（每个进程映射地址不同）
    static void* s_chunkBaseAddress;                ///< 数据区共享内存基地址（每个进程映射地址不同）
    static size_t s_managementMemorySize;           ///< 管理区共享内存大小
    static size_t s_chunkMemorySize;                ///< 数据区共享内存大小
    // ...
};
```

**关键点**：
- 这些静态变量是 **进程本地** 的（每个进程都有自己的副本）
- 它们记录的是 **本进程** 映射的虚拟地址
- 不同进程中这些变量的值可以不同

#### 3. 地址转换

当需要访问共享内存中的数据时，使用以下模式：

```cpp
// 相对指针 -> 绝对地址
void* absoluteAddr = s_managementBaseAddress + relativePointer.getOffset();

// 绝对地址 -> 相对指针
relativePointer.setOffset(absoluteAddr - s_managementBaseAddress);
```

## 双共享内存架构

我们使用两个独立的共享内存段：

### 管理区（`/zerocp_memory_management`）

- **存储内容**：freeList、ChunkManager 对象、元数据
- **基地址变量**：`s_managementBaseAddress`（进程本地）
- **大小**：`s_managementMemorySize`

### 数据区（`/zerocp_memory_chunk`）

- **存储内容**：所有 MemPool 的实际数据块（ChunkHeader + 用户数据）
- **基地址变量**：`s_chunkBaseAddress`（进程本地）
- **大小**：`s_chunkMemorySize`

## 内存布局示意图

```
进程 A 视图:
┌─────────────────────────────────────────┐
│ 管理区共享内存 @ 0x7f0000000000        │
│  - freeList                             │
│  - ChunkManager 对象                    │
│  - MemPool 元数据（使用 RelativePointer)│
└─────────────────────────────────────────┘
┌─────────────────────────────────────────┐
│ 数据区共享内存 @ 0x7f1000000000        │
│  - Chunk 1: [ChunkHeader][用户数据]    │
│  - Chunk 2: [ChunkHeader][用户数据]    │
│  - ...                                  │
└─────────────────────────────────────────┘

进程 B 视图:
┌─────────────────────────────────────────┐
│ 管理区共享内存 @ 0x7e0000000000        │  ← 地址不同，但内容相同
│  - freeList                             │
│  - ChunkManager 对象                    │
│  - MemPool 元数据（使用 RelativePointer)│
└─────────────────────────────────────────┘
┌─────────────────────────────────────────┐
│ 数据区共享内存 @ 0x7e1000000000        │  ← 地址不同，但内容相同
│  - Chunk 1: [ChunkHeader][用户数据]    │
│  - Chunk 2: [ChunkHeader][用户数据]    │
│  - ...                                  │
└─────────────────────────────────────────┘
```

## 为什么这样设计？

### ✅ 优点

1. **进程安全**：每个进程可以独立映射共享内存到不同地址
2. **数据一致性**：共享内存中使用相对偏移，保证多进程看到相同的数据结构
3. **灵活性**：进程可以在不同时间启动，映射到不同地址
4. **可扩展性**：容易添加更多共享内存段

### ⚠️ 注意事项

1. **绝对指针禁止**：永远不要在共享内存中存储绝对指针
2. **基地址必须同步**：每个进程在附加到共享内存时，必须正确设置 `s_managementBaseAddress` 和 `s_chunkBaseAddress`
3. **跨进程通信**：如果需要传递地址，必须转换为相对偏移量

## 实现清单

- [x] `MemPool` 使用 `RelativePointer<void> m_rawmemory`
- [x] `MemPoolManager` 静态成员分离管理区和数据区基地址
- [x] `createSharedInstance` 正确记录两个共享内存区域的基地址
- [x] `destroySharedInstance` 正确清理所有进程本地变量
- [x] 使用常量 `MGMT_SHM_NAME` 和 `CHUNK_SHM_NAME` 管理共享内存名称

## 参考

- `RelativePointer` 实现：`zerocp_daemon/memory/include/relative_pointer.hpp`
- `MemPool` 实现：`zerocp_daemon/memory/include/mempool.hpp`
- `MemPoolManager` 实现：`zerocp_daemon/memory/include/mempool_manager.hpp`

