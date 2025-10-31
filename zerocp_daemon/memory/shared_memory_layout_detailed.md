# 共享内存管理区详细分布图

## 概述

本文档详细描述了 ZeroCP 框架的共享内存架构，包括两个独立的共享内存段：
1. **管理区共享内存** (`/zerocp_memory_management`)：存储 MemPoolManager 对象和管理数据结构
2. **数据区共享内存** (`/zerocp_memory_chunk`)：存储实际的数据 chunks

---

## 一、共享内存段总览

```
┌─────────────────────────────────────────────────────────────────────┐
│                        进程虚拟地址空间                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │ 管理区共享内存 (/zerocp_memory_management)                  │    │
│  │ 映射地址：0x7f8000000000 (进程A) / 0x7e9000000000 (进程B)   │    │
│  │ 大小：managerObjSize + managementDataSize                   │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │ 数据区共享内存 (/zerocp_memory_chunk)                       │    │
│  │ 映射地址：0x7f0100000000 (进程A) / 0x7e9800000000 (进程B)   │    │
│  │ 大小：chunkSize (所有池的 chunks 总和)                      │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘

⚠️ 重要：不同进程映射同一共享内存段到不同虚拟地址，
   因此需要使用相对偏移量(offset)而非绝对指针进行跨进程通信！
```

---

## 二、管理区共享内存详细布局

### 2.1 整体结构

```
管理区内存布局 (/zerocp_memory_management)
═══════════════════════════════════════════════════════════════════════

起始地址：managementAddress (例如: 0x7f8000000000)
总大小：managementSize = managerObjSize + managementDataSize

┌───────────────────────────────────────────────────────────────────┐
│ [第1部分] MemPoolManager 对象本身                                  │
│ 起始偏移：0                                                        │
│ 大小：sizeof(MemPoolManager) ≈ 对齐到8字节                        │
├───────────────────────────────────────────────────────────────────┤
│ [第2部分] 管理数据结构（actualManagementStart 开始）               │
│ 起始偏移：managerObjSize                                           │
│ 大小：managementDataSize                                           │
│   ├─ [2.1] 数据池的 freeList 数组                                 │
│   ├─ [2.2] ChunkManager 对象数组                                  │
│   └─ [2.3] ChunkManagerPool 的 freeList                           │
└───────────────────────────────────────────────────────────────────┘
```

### 2.2 第1部分：MemPoolManager 对象详细结构

```cpp
┌───────────────────────────────────────────────────────────────────┐
│ MemPoolManager 对象 (placement new 构造)                          │
│ 地址：managementAddress                                            │
│ 大小：sizeof(MemPoolManager) ≈ 几百字节（对齐到8）                 │
├───────────────────────────────────────────────────────────────────┤
│                                                                    │
│ +0x00: m_config (引用，指向进程本地的配置对象)                     │
│        ⚠️ 注意：这是进程本地引用，不能跨进程使用！                  │
│                                                                    │
│ +0x08: m_mempools (vector<MemPool, 16>)                           │
│        ├─ m_size: 当前池数量（例如：3）                            │
│        ├─ m_capacity: 16 (固定容量)                               │
│        └─ m_data[16]: MemPool 对象数组                            │
│            ├─ [0] MemPool (池0，管理 128B chunks)                │
│            │   ├─ m_baseAddress (共享内存基地址)                  │
│            │   ├─ m_rawMemory (指向数据区中 chunks 的起始地址)    │
│            │   ├─ m_dataOffset (池首偏移，用于多进程通信)         │
│            │   ├─ m_chunkSize: 128                                │
│            │   ├─ m_totalChunks: 100                              │
│            │   ├─ m_freeList (MPMC_LockFree_List 对象)            │
│            │   │   ├─ m_head (原子指针)                           │
│            │   │   └─ m_indexMemory (指向 freeList 数组)          │
│            │   └─ m_pool_id: 0                                    │
│            │   大小：≈ 80-120 字节                                 │
│            │                                                       │
│            ├─ [1] MemPool (池1，管理 1024B chunks)               │
│            │   ├─ m_baseAddress                                   │
│            │   ├─ m_rawMemory                                     │
│            │   ├─ m_dataOffset                                    │
│            │   ├─ m_chunkSize: 1024                               │
│            │   ├─ m_totalChunks: 50                               │
│            │   ├─ m_freeList                                      │
│            │   └─ m_pool_id: 1                                    │
│            │                                                       │
│            ├─ [2] MemPool (池2，管理 4096B chunks)               │
│            │   └─ ... (类似结构)                                  │
│            │                                                       │
│            └─ [3..15] 未使用的 MemPool 槽位                       │
│                                                                    │
│ +0xXX: m_chunkManagerPool (vector<MemPool, 1>)                    │
│        ├─ m_size: 1                                                │
│        ├─ m_capacity: 1                                            │
│        └─ m_data[1]:                                               │
│            └─ [0] MemPool (管理 ChunkManager 对象)                │
│                ├─ m_baseAddress                                   │
│                ├─ m_rawMemory (指向 ChunkManager 对象数组)        │
│                ├─ m_chunkSize: align(sizeof(ChunkManager), 8)    │
│                ├─ m_totalChunks: totalChunks (所有池的chunks总和) │
│                ├─ m_freeList                                      │
│                └─ m_pool_id: 0                                    │
│                                                                    │
└───────────────────────────────────────────────────────────────────┘
```

### 2.3 第2部分：管理数据结构详细分布

管理数据结构从 `actualManagementStart = managementAddress + managerObjSize` 开始：

```
actualManagementStart (managementAddress + managerObjSize)
│
├─ [区块 2.1] 池0的 freeList 数组 ─────────────────────────────────
│  ┌─────────────────────────────────────────────────────────────┐
│  │ MPMC_LockFree_List 的索引数组                               │
│  │ 地址：freeListMemory[0]                                     │
│  │ 大小：align(MPMC_LockFree_List::requiredIndexMemorySize(   │
│  │          chunkCount), 8U)                                   │
│  ├─────────────────────────────────────────────────────────────┤
│  │ 例如：池0 (128B, 100个chunks)                               │
│  │ ─────────────────────────────────────────────────────────── │
│  │ +0x00: m_head (原子整数，指向首个空闲索引)                  │
│  │ +0x08: m_size (容量：100)                                   │
│  │ +0x10: m_invalidIndex (特殊值，表示无效)                    │
│  │ +0x18: uint32_t indices[100]:                              │
│  │        ├─ indices[0] = 0                                    │
│  │        ├─ indices[1] = 1                                    │
│  │        ├─ indices[2] = 2                                    │
│  │        └─ ...                                               │
│  │        └─ indices[99] = 99                                  │
│  │                                                             │
│  │ 总大小 ≈ 头部 + 100 * sizeof(uint32_t) = 24 + 400 = 424字节│
│  │ 对齐后 ≈ 432 字节                                           │
│  └─────────────────────────────────────────────────────────────┘
│
├─ [区块 2.2] 池1的 freeList 数组 ─────────────────────────────────
│  ┌─────────────────────────────────────────────────────────────┐
│  │ 例如：池1 (1024B, 50个chunks)                               │
│  │ ─────────────────────────────────────────────────────────── │
│  │ +0x00: m_head                                               │
│  │ +0x08: m_size (容量：50)                                    │
│  │ +0x10: m_invalidIndex                                       │
│  │ +0x18: uint32_t indices[50]                                │
│  │                                                             │
│  │ 总大小 ≈ 24 + 50 * 4 = 224 字节                            │
│  │ 对齐后 ≈ 224 字节                                           │
│  └─────────────────────────────────────────────────────────────┘
│
├─ [区块 2.3] 池2的 freeList 数组 ─────────────────────────────────
│  ┌─────────────────────────────────────────────────────────────┐
│  │ 例如：池2 (4096B, 20个chunks)                               │
│  │ ─────────────────────────────────────────────────────────── │
│  │ +0x00: m_head                                               │
│  │ +0x08: m_size (容量：20)                                    │
│  │ +0x10: m_invalidIndex                                       │
│  │ +0x18: uint32_t indices[20]                                │
│  │                                                             │
│  │ 总大小 ≈ 24 + 20 * 4 = 104 字节                            │
│  │ 对齐后 ≈ 104 字节                                           │
│  └─────────────────────────────────────────────────────────────┘
│
├─ [区块 2.4] ChunkManager 对象数组 ───────────────────────────────
│  ┌─────────────────────────────────────────────────────────────┐
│  │ ChunkManager 对象数组                                       │
│  │ 地址：chunkManagerMemory                                    │
│  │ 数量：totalChunks = 池0数量 + 池1数量 + 池2数量             │
│  │       = 100 + 50 + 20 = 170 个                             │
│  │ 大小：totalChunks * align(sizeof(ChunkManager), 8U)        │
│  ├─────────────────────────────────────────────────────────────┤
│  │                                                             │
│  │ ChunkManager[0] (56字节，8字节对齐 = 56字节):              │
│  │ ┌───────────────────────────────────────────────────────┐  │
│  │ │ +0x00: m_chunkHeader (RelativePointer<ChunkHeader>)   │  │
│  │ │        指向数据区中的某个 chunk 的 ChunkHeader         │  │
│  │ │ +0x08: m_referenceCounter (atomic<uint64_t>)          │  │
│  │ │        当前引用计数                                    │  │
│  │ │ +0x10: m_mempool (RelativePointer<MemPool>)           │  │
│  │ │        指向管理区中的 MemPool 对象                     │  │
│  │ │ +0x18: m_chunkManagementPool (RelativePointer)        │  │
│  │ │        指向 ChunkManagerPool                          │  │
│  │ │ +0x20: ... (其他成员)                                 │  │
│  │ └───────────────────────────────────────────────────────┘  │
│  │                                                             │
│  │ ChunkManager[1] (56字节):                                  │
│  │ ┌───────────────────────────────────────────────────────┐  │
│  │ │ ... (同上结构)                                         │  │
│  │ └───────────────────────────────────────────────────────┘  │
│  │                                                             │
│  │ ...                                                         │
│  │                                                             │
│  │ ChunkManager[169] (56字节):                                │
│  │ ┌───────────────────────────────────────────────────────┐  │
│  │ │ ... (同上结构)                                         │  │
│  │ └───────────────────────────────────────────────────────┘  │
│  │                                                             │
│  │ 总大小：170 * 56 = 9,520 字节                              │
│  └─────────────────────────────────────────────────────────────┘
│
└─ [区块 2.5] ChunkManagerPool 的 freeList 数组 ──────────────────
   ┌─────────────────────────────────────────────────────────────┐
   │ MPMC_LockFree_List 的索引数组                               │
   │ 地址：chunkMgrFreeListMemory                                │
   │ 大小：align(MPMC_LockFree_List::requiredIndexMemorySize(   │
   │         totalChunks), 8U)                                   │
   ├─────────────────────────────────────────────────────────────┤
   │ ChunkManagerPool freeList (170个ChunkManager对象)          │
   │ ─────────────────────────────────────────────────────────── │
   │ +0x00: m_head                                               │
   │ +0x08: m_size (容量：170)                                   │
   │ +0x10: m_invalidIndex                                       │
   │ +0x18: uint32_t indices[170]:                              │
   │        ├─ indices[0] = 0                                    │
   │        ├─ indices[1] = 1                                    │
   │        └─ ...                                               │
   │        └─ indices[169] = 169                                │
   │                                                             │
   │ 总大小 ≈ 24 + 170 * 4 = 704 字节                           │
   │ 对齐后 ≈ 704 字节                                           │
   └─────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════
管理区总大小汇总 (示例配置：3个池，170个chunks)：
───────────────────────────────────────────────────────────────────────
1. MemPoolManager 对象：         ≈ 500 字节
2. 池0 freeList (100个):        ≈ 432 字节
3. 池1 freeList (50个):         ≈ 224 字节
4. 池2 freeList (20个):         ≈ 104 字节
5. ChunkManager 对象数组:        ≈ 9,520 字节
6. ChunkManagerPool freeList:   ≈ 704 字节
───────────────────────────────────────────────────────────────────────
总计：                           ≈ 11,484 字节 ≈ 11.2 KB
═══════════════════════════════════════════════════════════════════════
```

---

## 三、数据区共享内存详细布局

### 3.1 整体结构

```
数据区内存布局 (/zerocp_memory_chunk)
═══════════════════════════════════════════════════════════════════════

起始地址：chunkMemoryAddress (例如: 0x7f0100000000)
总大小：chunkSize = Σ(每个池的 chunks 总大小)

┌───────────────────────────────────────────────────────────────────┐
│ [池0] 128B chunks 数组                                             │
│ dataOffset = 0                                                     │
│ 数量：100 个                                                       │
│ 单个大小：align(sizeof(ChunkHeader) + 128, 8) = align(48+128, 8) │
│         = 176 字节                                                 │
│ 总大小：176 * 100 = 17,600 字节                                    │
├───────────────────────────────────────────────────────────────────┤
│ [池1] 1024B chunks 数组                                            │
│ dataOffset = 17,600                                                │
│ 数量：50 个                                                        │
│ 单个大小：align(48 + 1024, 8) = 1,072 字节                        │
│ 总大小：1,072 * 50 = 53,600 字节                                   │
├───────────────────────────────────────────────────────────────────┤
│ [池2] 4096B chunks 数组                                            │
│ dataOffset = 17,600 + 53,600 = 71,200                             │
│ 数量：20 个                                                        │
│ 单个大小：align(48 + 4096, 8) = 4,144 字节                        │
│ 总大小：4,144 * 20 = 82,880 字节                                   │
└───────────────────────────────────────────────────────────────────┘

总计：17,600 + 53,600 + 82,880 = 154,080 字节 ≈ 150.5 KB
```

### 3.2 池0详细布局 (128B chunks)

```
池0起始地址：chunkMemoryAddress + 0
dataOffset: 0

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[0] - 偏移 0x0000 (地址：chunkMemoryAddress + 0)             │
│ ┌─────────────────────────────────────────────────────────────┐  │
│ │ ChunkHeader (48 字节)                                        │  │
│ │ ┌───────────────────────────────────────────────────────┐   │  │
│ │ │ +0x00: m_chunkSize: 128                               │   │  │
│ │ │ +0x08: m_userPayloadSize: 128                         │   │  │
│ │ │ +0x10: m_userPayloadOffset: 48                        │   │  │
│ │ │ +0x18: m_userHeaderId: 0                              │   │  │
│ │ │ +0x20: m_originId: ...                                │   │  │
│ │ │ +0x28: m_sequenceNumber: ...                          │   │  │
│ │ └───────────────────────────────────────────────────────┘   │  │
│ │                                                              │  │
│ │ User-Payload (128 字节)                                      │  │
│ │ ┌───────────────────────────────────────────────────────┐   │  │
│ │ │ 用户数据区域                                           │   │  │
│ │ │ 大小：128 字节                                         │   │  │
│ │ │ [可由用户填充任意数据]                                 │   │  │
│ │ └───────────────────────────────────────────────────────┘   │  │
│ └─────────────────────────────────────────────────────────────┘  │
│ 总大小：176 字节 (48 + 128)                                       │
└───────────────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[1] - 偏移 0x00B0 (地址：chunkMemoryAddress + 176)           │
│ ┌─────────────────────────────────────────────────────────────┐  │
│ │ ChunkHeader (48 字节)                                        │  │
│ │ User-Payload (128 字节)                                      │  │
│ └─────────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[2] - 偏移 0x0160 (地址：chunkMemoryAddress + 352)           │
│ ...                                                                │
└───────────────────────────────────────────────────────────────────┘

...

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[99] - 偏移 0x44B0 (地址：chunkMemoryAddress + 17,424)       │
│ ┌─────────────────────────────────────────────────────────────┐  │
│ │ ChunkHeader (48 字节)                                        │  │
│ │ User-Payload (128 字节)                                      │  │
│ └─────────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────┘

池0总大小：176 * 100 = 17,600 字节
```

### 3.3 池1详细布局 (1024B chunks)

```
池1起始地址：chunkMemoryAddress + 17,600
dataOffset: 17,600

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[0] - 偏移 0x44C0 (地址：chunkMemoryAddress + 17,600)        │
│ ┌─────────────────────────────────────────────────────────────┐  │
│ │ ChunkHeader (48 字节)                                        │  │
│ │ ┌───────────────────────────────────────────────────────┐   │  │
│ │ │ +0x00: m_chunkSize: 1024                              │   │  │
│ │ │ +0x08: m_userPayloadSize: 1024                        │   │  │
│ │ │ +0x10: m_userPayloadOffset: 48                        │   │  │
│ │ │ ... (其他字段)                                         │   │  │
│ │ └───────────────────────────────────────────────────────┘   │  │
│ │                                                              │  │
│ │ User-Payload (1024 字节)                                     │  │
│ │ ┌───────────────────────────────────────────────────────┐   │  │
│ │ │ 用户数据区域                                           │   │  │
│ │ │ 大小：1024 字节                                        │   │  │
│ │ └───────────────────────────────────────────────────────┘   │  │
│ └─────────────────────────────────────────────────────────────┘  │
│ 总大小：1,072 字节 (48 + 1024)                                    │
└───────────────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[1] - 偏移 0x48F0 (地址：chunkMemoryAddress + 18,672)        │
│ ...                                                                │
└───────────────────────────────────────────────────────────────────┘

...

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[49] - 偏移 0x11870 (地址：chunkMemoryAddress + 71,152)      │
│ ┌─────────────────────────────────────────────────────────────┐  │
│ │ ChunkHeader (48 字节)                                        │  │
│ │ User-Payload (1024 字节)                                     │  │
│ └─────────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────┘

池1总大小：1,072 * 50 = 53,600 字节
```

### 3.4 池2详细布局 (4096B chunks)

```
池2起始地址：chunkMemoryAddress + 71,200
dataOffset: 71,200

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[0] - 偏移 0x11600 (地址：chunkMemoryAddress + 71,200)       │
│ ┌─────────────────────────────────────────────────────────────┐  │
│ │ ChunkHeader (48 字节)                                        │  │
│ │ ┌───────────────────────────────────────────────────────┐   │  │
│ │ │ +0x00: m_chunkSize: 4096                              │   │  │
│ │ │ +0x08: m_userPayloadSize: 4096                        │   │  │
│ │ │ +0x10: m_userPayloadOffset: 48                        │   │  │
│ │ │ ... (其他字段)                                         │   │  │
│ │ └───────────────────────────────────────────────────────┘   │  │
│ │                                                              │  │
│ │ User-Payload (4096 字节)                                     │  │
│ │ ┌───────────────────────────────────────────────────────┐   │  │
│ │ │ 用户数据区域                                           │   │  │
│ │ │ 大小：4096 字节                                        │   │  │
│ │ └───────────────────────────────────────────────────────┘   │  │
│ └─────────────────────────────────────────────────────────────┘  │
│ 总大小：4,144 字节 (48 + 4096)                                    │
└───────────────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[1] - 偏移 0x12630 (地址：chunkMemoryAddress + 75,344)       │
│ ...                                                                │
└───────────────────────────────────────────────────────────────────┘

...

┌───────────────────────────────────────────────────────────────────┐
│ Chunk[19] - 偏移 0x25C70 (地址：chunkMemoryAddress + 154,032)     │
│ ┌─────────────────────────────────────────────────────────────┐  │
│ │ ChunkHeader (48 字节)                                        │  │
│ │ User-Payload (4096 字节)                                     │  │
│ └─────────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────┘

池2总大小：4,144 * 20 = 82,880 字节
```

---

## 四、多进程访问与地址计算

### 4.1 为什么需要 dataOffset？

```
问题：不同进程映射同一共享内存到不同虚拟地址

进程 A 的视角：
┌──────────────────────────────────────────────────────┐
│ 管理区映射到：0x7f8000000000                          │
│ 数据区映射到：0x7f0100000000                          │
│   ├─ 池0 起始：0x7f0100000000 (dataOffset = 0)       │
│   ├─ 池1 起始：0x7f0100004500 (dataOffset = 17,600)  │
│   └─ 池2 起始：0x7f0100011600 (dataOffset = 71,200)  │
└──────────────────────────────────────────────────────┘

进程 B 的视角：
┌──────────────────────────────────────────────────────┐
│ 管理区映射到：0x7e9000000000 ❌ 地址不同！            │
│ 数据区映射到：0x7e9800000000 ❌ 地址不同！            │
│   ├─ 池0 起始：0x7e9800000000 (dataOffset = 0)       │
│   ├─ 池1 起始：0x7e9800004500 (dataOffset = 17,600)  │
│   └─ 池2 起始：0x7e9800011600 (dataOffset = 71,200)  │
└──────────────────────────────────────────────────────┘

✅ 解决方案：使用 dataOffset（相对偏移量）
   - dataOffset 对所有进程都相同
   - 每个进程使用自己的基地址 + dataOffset 计算实际地址
```

### 4.2 地址计算公式

```cpp
// 获取池中第 N 个 chunk 的地址
void* getChunkAddress(uint32_t poolIndex, uint32_t chunkIndex)
{
    // 1. 获取本进程的数据区基地址
    void* chunkBaseAddr = MemPoolManager::getChunkBaseAddress();
    
    // 2. 获取池信息
    MemPool& pool = mempools[poolIndex];
    uint64_t dataOffset = pool.getDataOffset();  // 池首偏移（跨进程一致）
    
    // 3. 计算单个 chunk 的实际大小
    uint64_t actualChunkSize = align(sizeof(ChunkHeader) + pool.getChunkSize(), 8U);
    
    // 4. 计算最终地址
    void* chunkAddr = (char*)chunkBaseAddr + dataOffset + chunkIndex * actualChunkSize;
    
    return chunkAddr;
}
```

### 4.3 示例：跨进程定位同一个 chunk

```cpp
// 场景：进程 A 想告诉进程 B 某个 chunk 的位置

// ─────────────────────────────────────────────────────────────
// 进程 A：获取 chunk 并传递位置信息
// ─────────────────────────────────────────────────────────────
ChunkManager* chunkMgr = memPoolMgr->getChunk(512);  // 分配512字节

// 传递给进程 B 的信息（通过IPC/socket/pipe等）
struct ChunkLocation {
    uint32_t poolIndex;    // 1 (池1可以容纳512字节)
    uint32_t chunkIndex;   // 假设是 5 (池1的第5个chunk)
};

// 进程 A 的实际地址（仅供调试）：
// 0x7f0100000000 (基地址) + 17,600 (池1 dataOffset) + 5 * 1,072 = 0x7f0100005BC0

// ─────────────────────────────────────────────────────────────
// 进程 B：接收位置信息并定位 chunk
// ─────────────────────────────────────────────────────────────
ChunkLocation loc = receiveFromProcessA();  // poolIndex=1, chunkIndex=5

MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();
void* chunkBaseAddr = mgr->getChunkBaseAddress();  // 0x7e9800000000 (不同地址！)
MemPool& pool = mgr->getMemPools()[loc.poolIndex];

uint64_t dataOffset = pool.getDataOffset();  // 17,600 (相同！)
uint64_t actualChunkSize = 1,072;
void* chunkAddr = (char*)chunkBaseAddr + dataOffset + loc.chunkIndex * actualChunkSize;

// 进程 B 的实际地址：
// 0x7e9800000000 (基地址) + 17,600 (池1 dataOffset) + 5 * 1,072 = 0x7e9800005BC0

// ✅ 虽然基地址不同，但通过 dataOffset 计算，两个进程访问了相同的物理内存！
```

---

## 五、初始化流程详解

### 5.1 第一个进程（创建者）

```cpp
// 1. 计算内存大小
size_t managerObjSize = align(sizeof(MemPoolManager), 8U);
uint64_t managementDataSize = tempMgr.getManagementMemorySize();
uint64_t managementSize = managerObjSize + managementDataSize;
uint64_t chunkSize = tempMgr.getChunkMemorySize();

// 2. 创建共享内存
void* managementAddress = createShm("/zerocp_memory_management", managementSize);
void* chunkMemoryAddress = createShm("/zerocp_memory_chunk", chunkSize);

// 3. 在共享内存中构造 MemPoolManager
s_instance = new (managementAddress) MemPoolManager(config);

// 4. 创建 MemPoolAllocator 并布局管理区
MemPoolAllocator allocator(config, managementAddress);
void* actualManagementStart = (char*)managementAddress + managerObjSize;
allocator.ManagementMemoryLayout(
    actualManagementStart,
    managementDataSize,
    s_instance->m_mempools,           // 填充 MemPool 对象
    s_instance->m_chunkManagerPool    // 填充 ChunkManagerPool
);
// ManagementMemoryLayout 做的事：
//   - 为每个池分配 freeList 内存
//   - 调用 m_mempools.emplace_back() 构造 MemPool 对象
//   - 分配 ChunkManager 对象数组
//   - 分配 ChunkManagerPool 的 freeList

// 5. 布局数据区
allocator.ChunkMemoryLayout(
    chunkMemoryAddress,
    chunkSize,
    s_instance->m_mempools
);
// ChunkMemoryLayout 做的事：
//   - 为每个池分配 chunk 数据块
//   - 计算 dataOffset = chunkMemory - baseAddress
//   - 调用 pool.setRawMemory(chunkMemory, dataOffset)

// 6. 保存基地址（进程本地变量）
s_managementBaseAddress = managementAddress;
s_chunkBaseAddress = chunkMemoryAddress;
```

### 5.2 后续进程（附加者）

```cpp
// 1. 打开已存在的共享内存
void* managementAddress = openShm("/zerocp_memory_management");
void* chunkMemoryAddress = openShm("/zerocp_memory_chunk");

// 2. 直接使用共享内存中已存在的 MemPoolManager 对象
s_instance = static_cast<MemPoolManager*>(managementAddress);

// 3. 保存基地址（进程本地变量）
s_managementBaseAddress = managementAddress;
s_chunkBaseAddress = chunkMemoryAddress;

// ✅ 不需要重新布局！因为：
//    - MemPoolManager 对象已经在共享内存中
//    - 所有 MemPool 对象已经在共享内存中
//    - freeList、ChunkManager 数组都已经在共享内存中
//    - dataOffset 已经记录在每个 MemPool 中
// 
// ✅ 只需要使用本进程的基地址 + dataOffset 即可访问数据！
```

---

## 六、关键数据结构总结

### 6.1 MemPoolManager
- **位置**：管理区共享内存起始位置
- **大小**：≈ 500 字节（包含 vectors）
- **关键成员**：
  - `m_mempools`：vector<MemPool, 16>（数据池数组）
  - `m_chunkManagerPool`：vector<MemPool, 1>（ChunkManager 池）

### 6.2 MemPool
- **位置**：MemPoolManager 对象内部（vector 的 m_data 数组）
- **大小**：≈ 80-120 字节
- **关键成员**：
  - `m_rawMemory`：指向数据区中 chunks 的起始地址（进程相关）
  - `m_dataOffset`：池首偏移量（跨进程一致）✅
  - `m_freeList`：MPMC_LockFree_List（管理空闲 chunk 索引）

### 6.3 MPMC_LockFree_List
- **位置**：管理区共享内存（每个池一个）
- **大小**：24 + 4 * chunkCount 字节
- **功能**：无锁并发管理空闲 chunk 索引

### 6.4 ChunkManager
- **位置**：管理区共享内存（连续数组）
- **大小**：56 字节/个
- **数量**：等于所有池的 chunks 总数
- **功能**：管理 chunk 的引用计数和生命周期

### 6.5 Chunk (ChunkHeader + User-Payload)
- **位置**：数据区共享内存
- **大小**：sizeof(ChunkHeader) + userPayloadSize
- **ChunkHeader**：48 字节（包含元数据）
- **User-Payload**：用户数据区域

---

## 七、内存大小计算示例

假设配置：
- 池0：128B × 100 = 12,800 字节（header后17,600字节）
- 池1：1024B × 50 = 51,200 字节（header后53,600字节）
- 池2：4096B × 20 = 81,920 字节（header后82,880字节）

### 管理区大小
```
MemPoolManager 对象:        500 字节
池0 freeList:              432 字节
池1 freeList:              224 字节
池2 freeList:              104 字节
ChunkManager 数组:         9,520 字节 (170 × 56)
ChunkManagerPool freeList: 704 字节
────────────────────────────────────
总计:                      11,484 字节 ≈ 11.2 KB
```

### 数据区大小
```
池0 chunks:  17,600 字节 (176 × 100)
池1 chunks:  53,600 字节 (1,072 × 50)
池2 chunks:  82,880 字节 (4,144 × 20)
────────────────────────────────────
总计:        154,080 字节 ≈ 150.5 KB
```

### 总内存
```
管理区:  11.2 KB
数据区:  150.5 KB
────────────────────
总计:    161.7 KB
```

---

## 八、调试技巧

### 8.1 打印内存布局

```cpp
// 初始化完成后打印
ZEROCP_LOG(Info, "=== Shared Memory Layout ===");
ZEROCP_LOG(Info, "Management base: " << s_managementBaseAddress);
ZEROCP_LOG(Info, "Chunk base: " << s_chunkBaseAddress);

for (size_t i = 0; i < m_mempools.size(); ++i) {
    const auto& pool = m_mempools[i];
    ZEROCP_LOG(Info, "Pool[" << i << "]: "
               << "chunkSize=" << pool.getChunkSize()
               << ", count=" << pool.getTotalChunks()
               << ", dataOffset=" << pool.getDataOffset()
               << ", rawMemory=" << pool.getRawMemory());
}
```

### 8.2 验证地址计算

```cpp
// 验证池1的第5个chunk地址
uint32_t poolIdx = 1, chunkIdx = 5;
void* expected = (char*)s_chunkBaseAddress 
                 + m_mempools[poolIdx].getDataOffset()
                 + chunkIdx * 1072;
void* actual = /* 从 ChunkManager 获取 */;
assert(expected == actual);
```

---

## 九、注意事项

1. **地址空间隔离**：每个进程的虚拟地址不同，必须使用 `dataOffset` 计算
2. **对齐要求**：所有内存分配必须8字节对齐
3. **原子操作**：freeList 使用原子操作保证多进程/多线程安全
4. **引用计数**：ChunkManager 使用原子引用计数管理 chunk 生命周期
5. **初始化顺序**：先构造 MemPoolManager，再布局管理区，最后布局数据区
6. **清理顺序**：最后一个进程退出时需要调用 `destroySharedInstance()`

---

## 十、相关文件

- `mempool_manager.hpp/cpp`：MemPoolManager 实现
- `mempool_allocator.hpp/cpp`：内存布局逻辑
- `mempool.hpp/cpp`：MemPool 实现
- `chunk_manager.hpp/cpp`：ChunkManager 实现
- `mpmclockfreelist.hpp/cpp`：无锁并发链表
- `memory.md`：原始内存布局文档（80-117行描述数据区布局）
- `dataOffset_usage.md`：dataOffset 使用说明

