# MemPool 使用示例

本文档展示修改后的 `MemPool` 的各种使用方式。

---

## 示例一：在 vector 中直接存储 MemPool 对象（推荐）

这是**最推荐**的方式，对象直接存储在 `vector` 中，无需指针管理。

### 代码示例

```cpp
#include "mempool.hpp"
#include "vector.hpp"
#include "bump_allocator.hpp"

using namespace ZeroCP::Memory;

void example1_vector_storage() {
    // 准备共享内存
    void* sharedMemory = /* 共享内存基地址 */;
    BumpAllocator allocator(sharedMemory, memorySize);
    
    // 创建 vector 存储 MemPool 对象（最多 16 个）
    vector<MemPool, 16> mempools;
    
    // 为每个内存池准备参数
    for (size_t i = 0; i < poolConfigs.size(); ++i) {
        const auto& config = poolConfigs[i];
        
        // 分配 chunk 内存
        void* chunkMemory = allocator.allocate(
            config.chunkSize * config.chunkCount, 8).value();
        
        // 分配 freeList 内存
        uint64_t freeListSize = calculateFreeListSize(config.chunkCount);
        void* freeListMemory = allocator.allocate(freeListSize, 8).value();
        
        // ✅ 直接在 vector 中构造 MemPool 对象
        bool success = mempools.emplace_back(
            sharedMemory,           // baseAddress
            chunkMemory,            // rawMemory
            config.chunkSize,       // chunkSize
            config.chunkCount,      // chunkNums
            freeListMemory,         // freeListMemory
            i                       // pool_id
        );
        
        if (!success) {
            ZEROCP_LOG(Error, "Failed to add MemPool to vector");
            return;
        }
        
        ZEROCP_LOG(Info, "MemPool added successfully, size: ", mempools.size());
    }
    
    // ✅ 直接访问对象（无需指针）
    for (uint64_t i = 0; i < mempools.size(); ++i) {
        MemPool& pool = mempools[i];
        ZEROCP_LOG(Info, "Pool ", i, 
                   " - ChunkSize: ", pool.getChunkSize(),
                   " - TotalChunks: ", pool.getTotalChunks(),
                   " - FreeChunks: ", pool.getFreeChunks());
    }
}
```

### 优点分析

✅ **内存效率高**：对象直接存储在 vector 内部，无需额外的指针开销  
✅ **类型安全**：编译时检查，访问对象不需要解引用指针  
✅ **RAII 保证**：对象生命周期由 vector 管理，自动析构  
✅ **缓存友好**：对象连续存储，访问性能好  

---

## 示例二：在共享内存中创建 MemPool（使用 placement new）

当需要在**共享内存的特定位置**创建对象时使用此方式。

### 代码示例

```cpp
void example2_shared_memory_placement_new() {
    void* sharedMemory = /* 共享内存基地址 */;
    BumpAllocator allocator(sharedMemory, memorySize);
    
    // 为 MemPool 对象本身分配内存
    void* poolMemory = allocator.allocate(sizeof(MemPool), 8).value();
    
    // 为 chunk 数据分配内存
    void* chunkMemory = allocator.allocate(chunkSize * chunkCount, 8).value();
    
    // 为 freeList 分配内存
    void* freeListMemory = allocator.allocate(freeListSize, 8).value();
    
    // ✅ 使用 placement new 在指定位置构造对象
    MemPool* pool = new (poolMemory) MemPool(
        sharedMemory,       // baseAddress
        chunkMemory,        // rawMemory
        chunkSize,          // chunkSize
        chunkCount,         // chunkNums
        freeListMemory,     // freeListMemory
        0                   // pool_id
    );
    
    // ✅ 对象立即可用，无需 initialize
    ZEROCP_LOG(Info, "MemPool created in shared memory");
    ZEROCP_LOG(Info, "ChunkSize: ", pool->getChunkSize());
    ZEROCP_LOG(Info, "IsInitialized: ", pool->isInitialized());  // true
    
    // 清理时需要手动调用析构函数（如果在共享内存中）
    // pool->~MemPool();  // 显式析构（如果需要）
}
```

### 使用场景

✅ 需要在共享内存的特定位置创建对象  
✅ 对象地址必须固定（多进程访问）  
✅ 需要精确控制对象的内存布局  

---

## 示例三：兼容旧代码（默认构造 + initialize）

如果需要兼容旧代码，可以使用默认构造函数 + `initialize()` 方法。

### 代码示例

```cpp
void example3_legacy_two_phase_initialization() {
    void* sharedMemory = /* 共享内存基地址 */;
    BumpAllocator allocator(sharedMemory, memorySize);
    
    // 为 MemPool 对象分配内存
    void* poolMemory = allocator.allocate(sizeof(MemPool), 8).value();
    
    // ✅ 使用 placement new 调用默认构造函数
    MemPool* pool = new (poolMemory) MemPool();  // 默认构造
    
    // ⚠️ 此时对象是有效的，但未初始化
    ZEROCP_LOG(Info, "IsInitialized: ", pool->isInitialized());  // false
    
    // 分配其他需要的内存
    void* chunkMemory = allocator.allocate(chunkSize * chunkCount, 8).value();
    void* freeListMemory = allocator.allocate(freeListSize, 8).value();
    
    // ✅ 调用 initialize 完成初始化
    bool success = pool->initialize(
        sharedMemory,       // baseAddress
        chunkMemory,        // rawMemory
        chunkSize,          // chunkSize
        chunkCount,         // chunkNums
        freeListMemory,     // freeListMemory
        0                   // pool_id
    );
    
    if (!success) {
        ZEROCP_LOG(Error, "Failed to initialize MemPool");
        pool->~MemPool();  // 析构对象
        return;
    }
    
    // ✅ 初始化成功后才能使用
    ZEROCP_LOG(Info, "IsInitialized: ", pool->isInitialized());  // true
    ZEROCP_LOG(Info, "ChunkSize: ", pool->getChunkSize());
}
```

### 注意事项

⚠️ **对象在 `initialize()` 前处于部分可用状态**：
- 可以调用 `isInitialized()` 检查状态
- **不能**调用其他业务方法（如 `getChunkSize()`）

⚠️ **必须检查 `initialize()` 返回值**：
- 初始化可能失败（参数无效、已初始化等）
- 失败后需要手动析构对象

⚠️ **不推荐用于新代码**：
- 使用完整构造函数更安全、更简洁
- 仅用于兼容旧代码或特殊场景

---

## 示例四：错误的使用方式（对比）

### ❌ 错误一：memset + reinterpret_cast（旧代码的错误做法）

```cpp
void wrong_example1_memset_reinterpret() {
    void* poolMemory = allocator.allocate(sizeof(MemPool), 8).value();
    
    // ❌ 错误：memset 不会调用构造函数
    std::memset(poolMemory, 0, sizeof(MemPool));
    
    // ❌ 错误：reinterpret_cast 不会创建对象
    MemPool* pool = reinterpret_cast<MemPool*>(poolMemory);
    
    // ❌ 未定义行为：对象没有被构造
    pool->initialize(...);  // UB！
    
    // 为什么是 UB？
    // - std::atomic<uint32_t> m_usedChunk 没有被正确构造
    // - MPMC_LockFree_List m_freeIndices 没有被正确构造
    // - 访问未构造的对象是未定义行为
}
```

**正确做法**：
```cpp
// ✅ 必须使用 placement new 调用构造函数
MemPool* pool = new (poolMemory) MemPool();  // 默认构造
// 或
MemPool* pool = new (poolMemory) MemPool(args...);  // 带参数构造
```

---

### ❌ 错误二：忘记调用 initialize

```cpp
void wrong_example2_forget_initialize() {
    MemPool* pool = new (poolMemory) MemPool();  // 默认构造
    
    // ❌ 错误：忘记调用 initialize，直接使用
    uint64_t size = pool->getChunkSize();  // 返回 0（未初始化）
    
    // 正确做法：
    if (!pool->isInitialized()) {
        pool->initialize(...);
    }
}
```

---

### ❌ 错误三：重复初始化

```cpp
void wrong_example3_double_initialize() {
    MemPool* pool = new (poolMemory) MemPool();
    pool->initialize(args1...);  // 第一次初始化
    
    // ❌ 错误：重复初始化
    pool->initialize(args2...);  // 返回 false，初始化失败
    
    // initialize() 内部会检查：
    // if (m_initialized) {
    //     ZEROCP_LOG(Error, "MemPool already initialized");
    //     return false;
    // }
}
```

---

## 示例五：实际应用 - 修改 MemPoolAllocator

下面展示如何修改 `mempool_allocator.cpp` 中的代码。

### 原始代码（错误做法）

```cpp
// ❌ 旧代码（错误）
void* poolMemory = allocator.allocate(sizeof(MemPool), 8).value();
std::memset(poolMemory, 0, sizeof(MemPool));  // ❌ UB
MemPool* pool = reinterpret_cast<MemPool*>(poolMemory);  // ❌ UB
pool->initialize(...);  // ❌ UB
mempools.emplace_back(pool);  // 存储指针
```

### 修改方案一：使用完整构造函数（推荐）

```cpp
// ✅ 推荐方案：直接在 vector 中构造对象
bool MemPoolAllocator::ManagementMemoryLayout(void* baseAddress, uint64_t memorySize) {
    auto& manager = MemPoolManager::getInstance(m_config);
    auto& mempools = manager.getMemPools();  // vector<MemPool, 16>
    const auto& entries = m_config.m_memPoolEntries;
    
    BumpAllocator allocator(baseAddress, memorySize);
    
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        
        // 分配 chunk 内存
        void* chunkMemory = allocator.allocate(
            entry.m_poolSize * entry.m_poolCount, 8).value();
        
        // 分配 freeList 内存
        uint64_t freeListSize = calculateFreeListSize(entry.m_poolCount);
        void* freeListMemory = allocator.allocate(freeListSize, 8).value();
        
        // ✅ 直接在 vector 中构造 MemPool 对象
        bool success = mempools.emplace_back(
            baseAddress,            // baseAddress
            chunkMemory,            // rawMemory
            entry.m_poolSize,       // chunkSize
            entry.m_poolCount,      // chunkNums
            freeListMemory,         // freeListMemory
            i                       // pool_id
        );
        
        if (!success) {
            ZEROCP_LOG(Error, "Failed to emplace_back MemPool to vector");
            return false;
        }
        
        ZEROCP_LOG(Info, "MemPool initialized and added to vector");
    }
    
    return true;
}
```

### 修改方案二：使用默认构造 + initialize（兼容）

```cpp
// ⚠️ 兼容方案：保持指针方式，但使用正确的构造
bool MemPoolAllocator::ManagementMemoryLayout(void* baseAddress, uint64_t memorySize) {
    auto& manager = MemPoolManager::getInstance(m_config);
    auto& mempools = manager.getMemPools();  // vector<MemPool*, 16>
    const auto& entries = m_config.m_memPoolEntries;
    
    BumpAllocator allocator(baseAddress, memorySize);
    
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        
        // 为 MemPool 对象分配内存
        void* poolMemory = allocator.allocate(sizeof(MemPool), 8).value();
        
        // ✅ 使用 placement new 调用默认构造函数
        MemPool* pool = new (poolMemory) MemPool();
        
        // 分配其他内存
        void* chunkMemory = allocator.allocate(
            entry.m_poolSize * entry.m_poolCount, 8).value();
        uint64_t freeListSize = calculateFreeListSize(entry.m_poolCount);
        void* freeListMemory = allocator.allocate(freeListSize, 8).value();
        
        // ✅ 调用 initialize 完成初始化
        if (!pool->initialize(baseAddress, chunkMemory, entry.m_poolSize,
                             entry.m_poolCount, freeListMemory, i)) {
            ZEROCP_LOG(Error, "Failed to initialize MemPool");
            pool->~MemPool();  // 析构
            return false;
        }
        
        // 存储指针
        if (!mempools.emplace_back(pool)) {
            ZEROCP_LOG(Error, "Failed to add MemPool pointer to vector");
            return false;
        }
    }
    
    return true;
}
```

---

## 总结：三种方式对比

| 方式 | 代码行数 | 类型安全 | 性能 | 推荐度 | 适用场景 |
|------|---------|---------|------|--------|---------|
| **完整构造 + vector 存对象** | ⭐⭐⭐⭐⭐ 最少 | ✅ 完全 | ✅ 最好 | ⭐⭐⭐⭐⭐ | **新代码首选** |
| **placement new + 完整构造** | ⭐⭐⭐⭐ 较少 | ✅ 完全 | ✅ 好 | ⭐⭐⭐⭐ | 共享内存固定位置 |
| **默认构造 + initialize** | ⭐⭐⭐ 较多 | ⚠️ 运行时检查 | ⭐⭐⭐ 一般 | ⭐⭐ | 兼容旧代码 |
| **memset + reinterpret_cast** | ⭐⭐ 多 | ❌ 无 | ❌ UB | ❌❌❌ | **绝对禁止使用** |

---

## 关键要点

1. **必须使用构造函数创建对象**
   - `new`、`placement new`、`emplace_back` 都会调用构造函数
   - `memset` + `reinterpret_cast` 是**未定义行为**

2. **完整构造函数是最佳实践**
   - 对象构造完成即可用
   - 编译时类型安全
   - 符合 RAII 原则

3. **默认构造 + initialize 可用于兼容**
   - 但需要运行时检查 `isInitialized()`
   - 容易出错（忘记 initialize）

4. **推荐在 vector 中直接存储对象**
   - 性能更好（缓存友好）
   - 无需指针管理
   - 类型安全







