# MemPool 初始化指南

## 概述

本文档说明如何初始化 `MemPool` 中的 `m_chunkSize` 以及整个内存池系统的初始化流程。

## 初始化流程

### 1. 配置阶段

首先，通过 `MemPoolConfig` 配置内存池参数：

```cpp
// 创建配置
MemPoolConfig config;

// 添加内存池配置项
// 参数1: m_poolSize (chunk 大小)
// 参数2: m_poolCount (chunk 数量)
config.addMemPoolEntry(128, 10000);   // 128 字节 chunk，10000 个
config.addMemPoolEntry(1024, 5000);   // 1024 字节 chunk，5000 个
config.addMemPoolEntry(4096, 1000);   // 4096 字节 chunk，1000 个
```

### 2. 创建 MemPoolAllocator

```cpp
// 使用配置创建 allocator
MemPoolAllocator allocator(config);
```

### 3. 创建共享内存

```cpp
// 创建共享内存
void* baseAddress = allocator.createSharedMemory(
    "/my_shm",                    // 共享内存名称
    AccessMode::READ_WRITE,       // 访问模式
    OpenMode::OPEN_OR_CREATE,     // 打开模式
    Perms(0600)                   // 权限
);

if (baseAddress == nullptr) {
    // 处理错误
    return;
}
```

### 4. 布局和初始化内存

```cpp
// 获取总内存大小
auto& manager = MemPoolManager::getInstance(config);
uint64_t totalSize = manager.getTotalMemorySize();

// 布局内存并初始化所有内存池
if (!allocator.layoutMemory(baseAddress, totalSize)) {
    // 处理错误
    return;
}
```

## MemPool::initialize() 详解

`MemPool::initialize()` 方法用于初始化单个内存池，设置以下关键参数：

```cpp
bool MemPool::initialize(
    void* rawMemory,        // 原始内存地址（chunk 数据存储区）
    uint64_t chunkSize,     // 单个 chunk 的大小 (即 m_chunkSize)
    uint32_t chunkNums,     // chunk 的数量 (即 m_chunkNums)
    void* freeListMemory    // 空闲索引链表的内存地址
) noexcept;
```

### 参数说明

- **rawMemory**: 用于存储所有 chunk 数据的内存区域起始地址
- **chunkSize**: 从 `MemPoolConfig::MemPoolEntry::m_poolSize` 获取，表示每个 chunk 的大小
- **chunkNums**: 从 `MemPoolConfig::MemPoolEntry::m_poolCount` 获取，表示 chunk 的数量
- **freeListMemory**: 用于存储空闲索引链表的内存区域

### 初始化步骤

`MemPool::initialize()` 内部执行以下操作：

1. **验证参数**：检查内存地址和参数有效性
2. **设置成员变量**：
   ```cpp
   m_rawmemory = rawMemory;
   m_chunkSize = chunkSize;  // ← 这里初始化 m_chunkSize
   m_chunkNums = chunkNums;
   m_usedChunk.store(0);
   ```
3. **初始化空闲索引链表**：将所有索引 (0 到 chunkNums-1) 放入 `m_freeIndices`

## layoutMemory() 的工作流程

`MemPoolAllocator::layoutMemory()` 负责在共享内存上布局所有内存池：

```
共享内存布局：
┌────────────────────────────────────────────────────────┐
│ Pool 0: Chunk Data (128B × 10000)                     │
├────────────────────────────────────────────────────────┤
│ Pool 0: Free List Indices                             │
├────────────────────────────────────────────────────────┤
│ Pool 1: Chunk Data (1024B × 5000)                     │
├────────────────────────────────────────────────────────┤
│ Pool 1: Free List Indices                             │
├────────────────────────────────────────────────────────┤
│ Pool 2: Chunk Data (4096B × 1000)                     │
├────────────────────────────────────────────────────────┤
│ Pool 2: Free List Indices                             │
├────────────────────────────────────────────────────────┤
│ ChunkManager Objects (totalChunkCount)                │
├────────────────────────────────────────────────────────┤
│ ChunkManager Pool: Free List Indices                  │
└────────────────────────────────────────────────────────┘
```

### 详细步骤

#### 第一步：初始化数据 chunk 内存池

对于配置中的每个 `MemPoolEntry`：

```cpp
for (size_t i = 0; i < entries.size(); ++i) {
    const auto& entry = entries[i];  // MemPoolEntry
    auto& pool = mempools[i];        // MemPool
    
    // 1. 分配 chunk 数据内存
    uint64_t chunkDataSize = entry.m_poolSize * entry.m_poolCount;
    void* chunkMemory = allocator.allocate(chunkDataSize, 8);
    
    // 2. 分配空闲索引链表内存
    uint64_t freeListSize = MPMC_LockFree_List::requiredIndexMemorySize(entry.m_poolCount);
    void* freeListMemory = allocator.allocate(freeListSize, 8);
    
    // 3. 初始化内存池（这里设置 m_chunkSize）
    pool.initialize(
        chunkMemory,           // 原始内存
        entry.m_poolSize,      // chunk 大小 ← 来自配置
        entry.m_poolCount,     // chunk 数量 ← 来自配置
        freeListMemory         // 空闲链表内存
    );
}
```

#### 第二步：初始化 ChunkManager 对象池

```cpp
// 计算需要的 ChunkManager 数量（等于所有 chunk 的总数）
uint32_t totalChunkCount = 0;
for (const auto& entry : entries) {
    totalChunkCount += entry.m_poolCount;
}

// 初始化 ChunkManager 池
auto& mgmtPool = chunkManagerPool[0];

// 分配内存并初始化
void* chunkManagerMemory = allocator.allocate(
    sizeof(ChunkManager) * totalChunkCount, 8
);
void* mgmtFreeListMemory = allocator.allocate(
    MPMC_LockFree_List::requiredIndexMemorySize(totalChunkCount), 8
);

mgmtPool.initialize(
    chunkManagerMemory,
    sizeof(ChunkManager),  // ChunkManager 对象大小作为 chunkSize
    totalChunkCount,       // ChunkManager 对象数量
    mgmtFreeListMemory
);
```

## 关键概念

### m_chunkSize 的来源

`m_chunkSize` 的值来自于 `MemPoolConfig::MemPoolEntry::m_poolSize`：

- 对于**数据 chunk 池**：`m_chunkSize` = 用户配置的 chunk 大小（如 128、1024、4096）
- 对于 **ChunkManager 池**：`m_chunkSize` = `sizeof(ChunkManager)`

### MemPool 的双重用途

`MemPool` 类可以管理两种类型的内存池：

1. **数据 chunk 池**：存储实际的用户数据
2. **ChunkManager 对象池**：存储 `ChunkManager` 管理对象

两者使用相同的 `MemPool` 类，只是 `m_chunkSize` 不同。

## 使用示例

完整的使用示例：

```cpp
#include "mempool_allocator.hpp"
#include "mempool_config.hpp"
#include "mempool_manager.hpp"

int main() {
    // 1. 创建配置
    MemPoolConfig config;
    config.addMemPoolEntry(128, 10000);
    config.addMemPoolEntry(1024, 5000);
    config.addMemPoolEntry(4096, 1000);
    
    // 2. 创建 allocator
    MemPoolAllocator allocator(config);
    
    // 3. 创建共享内存
    void* baseAddress = allocator.createSharedMemory(
        "/my_shm",
        AccessMode::READ_WRITE,
        OpenMode::OPEN_OR_CREATE,
        Perms(0600)
    );
    
    if (baseAddress == nullptr) {
        std::cerr << "Failed to create shared memory" << std::endl;
        return 1;
    }
    
    // 4. 布局和初始化内存池
    auto& manager = MemPoolManager::getInstance(config);
    uint64_t totalSize = manager.getTotalMemorySize();
    
    if (!allocator.layoutMemory(baseAddress, totalSize)) {
        std::cerr << "Failed to layout memory" << std::endl;
        return 1;
    }
    
    // 5. 现在所有 MemPool 的 m_chunkSize 都已正确初始化
    auto& mempools = manager.getMemPools();
    
    std::cout << "Memory pools initialized:" << std::endl;
    for (size_t i = 0; i < mempools.size(); ++i) {
        const auto& pool = mempools[i];
        std::cout << "  Pool " << i << ": "
                  << "chunkSize=" << pool.getChunkSize() << ", "
                  << "totalChunks=" << pool.getTotalChunks() << ", "
                  << "freeChunks=" << pool.getFreeChunks() << std::endl;
    }
    
    return 0;
}
```

## 总结

**如何初始化 `MemPool` 中的 `m_chunkSize`**：

1. 在 `MemPoolConfig` 中配置 `MemPoolEntry`，指定 `m_poolSize`（chunk 大小）
2. 调用 `MemPoolAllocator::layoutMemory()`
3. `layoutMemory()` 内部调用 `MemPool::initialize()`，将 `m_poolSize` 传递给 `chunkSize` 参数
4. `MemPool::initialize()` 将 `chunkSize` 赋值给 `m_chunkSize` 成员变量

这样就完成了 `m_chunkSize` 的初始化过程。


