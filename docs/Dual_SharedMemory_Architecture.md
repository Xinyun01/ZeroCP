# 双共享内存架构设计

## 概述

MemPoolManager 现在采用**双共享内存区域**的架构设计，将内存池的管理区和数据区分离到两个独立的共享内存中。

## 架构组件

### 1. MemPoolManager (进程本地对象)

- **位置**: 每个进程的堆内存
- **作用**: 管理器实例，包含配置引用和内存池vector
- **生命周期**: 随进程启动/退出

```cpp
class MemPoolManager {
private:
    const MemPoolConfig& m_config;          // 配置引用
    vector<MemPool, 16> m_mempools;         // 数据池（在共享内存中）
    vector<MemPool, 1> m_chunkManagerPool;  // ChunkManager池（在共享内存中）
    
    static MemPoolManager* s_instance;      // 单例指针
};
```

### 2. 管理区共享内存 (`/zerocp_memory_management`)

**内容**:
- 所有 MemPool 的 freeList（MPMC无锁链表索引）
- 所有 ChunkManager 对象
- ChunkManagerPool 的 freeList

**大小计算**:
```cpp
uint64_t managementSize = 
    Σ(每个Pool的freeList大小) +
    (总chunk数 × ChunkManager对象大小) +
    ChunkManagerPool的freeList大小;
```

### 3. 数据区共享内存 (`/zerocp_memory_chunk`)

**内容**:
- 所有 MemPool 的实际数据块（chunks）

**大小计算**:
```cpp
uint64_t chunkSize = 
    Σ(每个Pool: chunkSize × chunkCount);
```

## 初始化流程

### `createSharedInstance(config)`:

```
1. 计算内存大小
   ├─ 创建临时 MemPoolManager
   ├─ 计算 managementSize
   └─ 计算 chunkSize

2. 创建管理区共享内存
   └─ PosixShmProvider("/zerocp_memory_management", managementSize)

3. 创建数据区共享内存
   └─ PosixShmProvider("/zerocp_memory_chunk", chunkSize)

4. 使用信号量保证单次初始化
   ├─ sem_open("/zerocp_init_sem", O_CREAT | O_EXCL)
   └─ 第一个进程创建，其他进程打开

5. 第一个进程初始化
   ├─ 创建本地 MemPoolManager 实例
   ├─ 使用 MemPoolAllocator 布局管理区
   ├─ 使用 MemPoolAllocator 布局数据区
   └─ 存储到 s_instance

6. 其他进程附加
   ├─ 创建本地 MemPoolManager 实例
   └─ 从共享内存恢复状态（TODO）

7. 解锁信号量
```

## 内存布局示意图

```
进程A内存:                         进程B内存:
┌────────────────────┐            ┌────────────────────┐
│ MemPoolManager     │            │ MemPoolManager     │
│   s_instance  ────────┐         │   s_instance  ────────┐
│   m_config (引用)  │  │         │   m_config (引用)  │  │
│   m_mempools       │  │         │   m_mempools       │  │
│   m_chunkMgrPool   │  │         │   m_chunkMgrPool   │  │
└────────────────────┘  │         └────────────────────┘  │
                        │                                 │
                        └──────────────┬──────────────────┘
                                       │
                                       ▼
            ┌──────────────────────────────────────────────┐
            │   共享内存1: /zerocp_memory_management       │
            ├──────────────────────────────────────────────┤
            │ Pool[0] freeList (索引数组)                  │
            │ Pool[1] freeList (索引数组)                  │
            │ Pool[2] freeList (索引数组)                  │
            │ ...                                          │
            ├──────────────────────────────────────────────┤
            │ ChunkManager[0] 对象                         │
            │ ChunkManager[1] 对象                         │
            │ ChunkManager[...] 对象                       │
            ├──────────────────────────────────────────────┤
            │ ChunkManagerPool freeList                    │
            └──────────────────────────────────────────────┘

            ┌──────────────────────────────────────────────┐
            │   共享内存2: /zerocp_memory_chunk            │
            ├──────────────────────────────────────────────┤
            │ Pool[0] chunks (1024 bytes × 1000)           │
            │ Pool[1] chunks (4096 bytes × 500)            │
            │ Pool[2] chunks (8192 bytes × 100)            │
            │ ...                                          │
            └──────────────────────────────────────────────┘
```

## 优势

1. **物理分离**: 管理数据和实际数据分离，便于独立管理
2. **灵活扩展**: 可以独立扩展管理区或数据区
3. **错误隔离**: 一个区域的问题不会直接影响另一个区域
4. **多进程共享**: 多个进程可以映射同样的共享内存
5. **独立保护**: 可以对两个区域设置不同的权限

## 清理流程

### `destroySharedInstance()`:

```
1. 删除本地实例
   └─ delete s_instance

2. 清空静态变量
   ├─ s_sharedMemoryAddress = nullptr
   └─ s_sharedMemorySize = 0

3. 关闭信号量
   └─ sem_close(s_initSemaphore)

4. 取消链接信号量
   └─ sem_unlink("/zerocp_init_sem")

5. 共享内存自动清理
   └─ PosixShmProvider 析构函数处理
```

## 注意事项

1. **配置一致性**: 所有进程必须使用相同的配置创建实例
2. **初始化顺序**: 第一个进程负责初始化，其他进程附加
3. **生命周期**: 共享内存在所有进程退出后才真正释放
4. **错误处理**: 初始化失败时需要正确清理已创建的资源

## 未来改进

1. 在共享内存中保存配置和元数据，支持其他进程恢复状态
2. 支持动态扩展共享内存大小
3. 添加版本检查，防止不兼容的进程附加
4. 实现更完善的错误恢复机制

