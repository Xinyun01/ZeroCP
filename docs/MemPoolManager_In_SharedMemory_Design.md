# MemPoolManager 在共享内存中的架构设计

## 一、设计目标

将整个 `MemPoolManager` 对象放入共享内存，使其可以被多个进程共享访问。

---

## 二、当前架构 vs 新架构

### 当前架构（进程本地）
```
┌─────────────────────────────────────────┐
│          进程 A                          │
│  ┌───────────────────────────────────┐  │
│  │  MemPoolManager (堆上 new)       │  │
│  │  - m_config (引用)               │  │
│  │  - m_mempools (vector)           │  │
│  │  - m_chunkManagerPool (vector)   │  │
│  │  - s_instance (static)           │  │
│  └───────────────────────────────────┘  │
│           ↓                              │
│  ┌───────────────────────────────────┐  │
│  │  共享内存                          │  │
│  │  - 管理区                          │  │
│  │  - 数据区                          │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘

问题：进程 B 无法访问进程 A 的 MemPoolManager 对象
```

### 新架构（共享内存）
```
┌─────────────────────────────────────────┐
│          进程 A                          │
│  ┌───────────────────────────────────┐  │
│  │  s_instance (RelativePointer)    │  │ ← 指向共享内存中的 MemPoolManager
│  └───────────────────────────────────┘  │
│           ↓                              │
│  ┌───────────────────────────────────┐  │
│  │  共享内存                          │  │
│  │  ┌─────────────────────────────┐  │  │
│  │  │ MemPoolManager 对象         │  │  │ ← MemPoolManager 在共享内存中
│  │  │  - m_mempools (vector)      │  │  │
│  │  │  - m_chunkManagerPool       │  │  │
│  │  └─────────────────────────────┘  │  │
│  │  - 管理区                          │  │
│  │  - 数据区                          │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
           ↑
┌─────────────────────────────────────────┐
│          进程 B                          │
│  ┌───────────────────────────────────┐  │
│  │  s_instance (RelativePointer)    │  │ ← 同样指向共享内存
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘

优势：进程 A 和 B 共享同一个 MemPoolManager 实例
```

---

## 三、内存布局设计

### 共享内存总布局
```
┌──────────────────────────────────────────────────────────┐
│                  共享内存总空间                           │
│                                                           │
│  ┌────────────────────────────────────────────────────┐  │
│  │ 1. MemPoolManager 对象区                            │  │
│  │    sizeof(MemPoolManager) 对齐到 8 字节              │  │
│  │    - m_mempools: vector<MemPool, 16>                │  │
│  │    - m_chunkManagerPool: vector<MemPool, 1>         │  │
│  │    - m_config: 需要特殊处理（见下文）                 │  │
│  └────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────┐  │
│  │ 2. MemPoolConfig 对象区                             │  │
│  │    存储配置数据（m_memPoolEntries）                  │  │
│  └────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────┐  │
│  │ 3. 管理区（Management Memory）                       │  │
│  │    - Pool 0 的 freeList                             │  │
│  │    - Pool 1 的 freeList                             │  │
│  │    - ...                                            │  │
│  │    - ChunkManager 对象数组                          │  │
│  │    - ChunkManagerPool 的 freeList                   │  │
│  └────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────┐  │
│  │ 4. 数据区（Data Memory）                             │  │
│  │    - Pool 0 的所有 chunks                           │  │
│  │    - Pool 1 的所有 chunks                           │  │
│  │    - ...                                            │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

### 大小计算公式
```cpp
总共享内存大小 = 
    align(sizeof(MemPoolManager), 8)        // MemPoolManager 对象
    + align(sizeof(MemPoolConfig), 8)       // MemPoolConfig 对象
    + getManagementMemorySize()              // 管理区
    + getChunkMemorySize()                   // 数据区（原来叫管理区，命名有误）
```

---

## 四、关键设计问题与解决方案

### 问题 1：`m_config` 是引用类型

**原问题**：
```cpp
class MemPoolManager {
    const MemPoolConfig& m_config;  // ❌ 引用类型不能在共享内存中
};
```

**解决方案 A：改为 RelativePointer**
```cpp
class MemPoolManager {
    RelativePointer<const MemPoolConfig> m_config;  // ✅ 指向共享内存中的配置
};
```

**解决方案 B：直接嵌入**（如果 MemPoolConfig 可以完全在共享内存中）
```cpp
class MemPoolManager {
    MemPoolConfig m_config;  // ✅ 直接包含
};
```

### 问题 2：`MemPoolConfig::m_memPoolEntries` 使用 `std::vector`

**原问题**：
```cpp
struct MemPoolConfig {
    std::vector<MemPoolEntry> m_memPoolEntries;  // ❌ std::vector 不能在共享内存中
};
```

**解决方案：改为 ZeroCP::vector**
```cpp
struct MemPoolConfig {
    ZeroCP::vector<MemPoolEntry, 16> m_memPoolEntries;  // ✅ 固定容量，可在共享内存
};
```

### 问题 3：单例模式的 `s_instance` 指针

**原问题**：
```cpp
class MemPoolManager {
    static MemPoolManager* s_instance;  // ❌ 普通指针，不同进程地址不同
};
```

**解决方案：改为 RelativePointer**
```cpp
class MemPoolManager {
    static RelativePointer<MemPoolManager> s_instance;  // ✅ 相对偏移
};
```

### 问题 4：`std::mutex` 不能在共享内存中

**原问题**：
```cpp
class MemPoolManager {
    static std::mutex s_mutex;  // ❌ std::mutex 不能跨进程
};
```

**解决方案：使用进程间互斥锁**
```cpp
// 方案 A：使用 POSIX 进程间信号量
class MemPoolManager {
    static sem_t* s_semaphore;  // 命名信号量
};

// 方案 B：使用共享内存中的 atomic + spin lock
class MemPoolManager {
    std::atomic_flag m_lock;  // 在共享内存中的自旋锁
};

// 方案 C：使用 pthread 进程间互斥锁
class MemPoolManager {
    pthread_mutex_t m_mutex;  // 设置为 PTHREAD_PROCESS_SHARED
};
```

---

## 五、修改后的类定义

### 1. MemPoolConfig 修改

```cpp
// 修改前
struct MemPoolConfig {
    std::vector<MemPoolEntry> m_memPoolEntries;  // ❌
};

// 修改后
struct MemPoolConfig {
    ZeroCP::vector<MemPoolEntry, 16> m_memPoolEntries;  // ✅ 最多 16 个 Pool
};
```

### 2. MemPoolManager 修改

```cpp
#include "relative_pointer.hpp"
#include <pthread.h>

namespace ZeroCP {
namespace Memory {

class MemPoolManager {
public:
    // ==================== 单例模式 ====================
    
    /// @brief 获取或创建共享内存中的实例
    /// @param config 内存池配置
    /// @return MemPoolManager 实例指针
    static MemPoolManager* getInstance(const MemPoolConfig& config) noexcept;
    
    /// @brief 获取已初始化的实例
    static MemPoolManager* getInstanceIfInitialized() noexcept;
    
    /// @brief placement new 构造函数（在共享内存中构造）
    /// @param sharedMemoryBase 共享内存基地址
    /// @param config 配置对象（在共享内存中）
    static MemPoolManager* constructInPlace(void* address, 
                                             void* sharedMemoryBase,
                                             const MemPoolConfig* config) noexcept;
    
    // 禁用拷贝和移动
    MemPoolManager(const MemPoolManager&) = delete;
    MemPoolManager(MemPoolManager&&) = delete;
    MemPoolManager& operator=(const MemPoolManager&) = delete;
    MemPoolManager& operator=(MemPoolManager&&) = delete;
    
    ~MemPoolManager() noexcept;

    // ==================== 内存大小计算 ====================
    
    /// @brief 计算 MemPoolManager 对象本身需要的内存
    static uint64_t getManagerObjectSize() noexcept {
        return align(sizeof(MemPoolManager), 8);
    }
    
    /// @brief 计算 MemPoolConfig 对象需要的内存
    static uint64_t getConfigObjectSize() noexcept {
        return align(sizeof(MemPoolConfig), 8);
    }
    
    /// @brief 计算管理区需要的内存大小
    uint64_t getManagementMemorySize() const noexcept;
    
    /// @brief 计算数据区需要的内存大小
    uint64_t getChunkMemorySize() const noexcept;
    
    /// @brief 计算总共需要的内存大小（包括 MemPoolManager 对象本身）
    uint64_t getTotalMemorySize() const noexcept;

    // ==================== 核心分配/释放接口 ====================
    
    ChunkManager* getChunk(uint64_t size) noexcept;
    bool releaseChunk(ChunkManager* chunkManager) noexcept;
    void printAllPoolStats() const noexcept;
    
    // ==================== 访问器接口 ====================
    
    vector<MemPool, 16>& getMemPools() noexcept;
    vector<MemPool, 1>& getChunkManagerPool() noexcept;
    
private:
    // 私有构造函数（用于 placement new）
    explicit MemPoolManager(void* sharedMemoryBase, 
                            const MemPoolConfig* config) noexcept;

    // ==================== 成员变量 ====================
    
    void* m_sharedMemoryBase;                       ///< 共享内存基地址（用于 RelativePointer）
    RelativePointer<const MemPoolConfig> m_config;  ///< 配置相对指针
    vector<MemPool, 16> m_mempools;                 ///< 数据 chunk 池（最多16个）
    vector<MemPool, 1> m_chunkManagerPool;          ///< ChunkManager 对象池
    pthread_mutex_t m_lock;                         ///< 进程间互斥锁
    
    // ==================== 静态成员（进程本地） ====================
    
    static void* s_sharedMemoryAddress;             ///< 共享内存映射地址（每个进程不同）
    static size_t s_sharedMemorySize;               ///< 共享内存大小
    static sem_t* s_initSemaphore;                  ///< 初始化信号量（命名信号量）
};

} // namespace Memory
} // namespace ZeroCP
```

---

## 六、初始化流程

### 6.1 创建共享内存并构造 MemPoolManager

```cpp
bool MemPoolManager::createSharedInstance(const MemPoolConfig& config) noexcept
{
    // 1. 计算总共需要的共享内存大小
    uint64_t totalSize = getManagerObjectSize()
                       + getConfigObjectSize()
                       + calculateManagementMemorySize(config)
                       + calculateChunkMemorySize(config);
    
    // 2. 创建或打开共享内存
    PosixShmProvider shmProvider(
        Name_t("/zerocp_mempool_manager"),
        totalSize,
        AccessMode::ReadWrite,
        OpenMode::OpenOrCreate,
        Perms::OwnerAll
    );
    
    auto result = shmProvider.createMemory();
    if (!result.has_value()) {
        ZEROCP_LOG(Error, "Failed to create shared memory");
        return false;
    }
    
    void* shmBase = result.value();
    s_sharedMemoryAddress = shmBase;
    s_sharedMemorySize = totalSize;
    
    // 3. 使用命名信号量保证只初始化一次
    s_initSemaphore = sem_open("/zerocp_init_sem", O_CREAT | O_EXCL, 0644, 1);
    bool isFirstProcess = (s_initSemaphore != SEM_FAILED);
    
    if (!isFirstProcess) {
        // 不是第一个进程，打开已存在的信号量
        s_initSemaphore = sem_open("/zerocp_init_sem", 0);
    }
    
    sem_wait(s_initSemaphore);  // 加锁
    
    // 4. 在共享内存中布局各个区域
    void* currentPtr = shmBase;
    
    // 4.1 MemPoolManager 对象区
    void* managerAddr = currentPtr;
    currentPtr = static_cast<char*>(currentPtr) + getManagerObjectSize();
    
    // 4.2 MemPoolConfig 对象区
    void* configAddr = currentPtr;
    currentPtr = static_cast<char*>(currentPtr) + getConfigObjectSize();
    
    // 4.3 拷贝配置到共享内存
    if (isFirstProcess) {
        MemPoolConfig* sharedConfig = new (configAddr) MemPoolConfig();
        // 拷贝 config 的内容到 sharedConfig
        for (const auto& entry : config.m_memPoolEntries) {
            sharedConfig->m_memPoolEntries.emplace_back(entry);
        }
    }
    
    // 4.4 在共享内存中构造 MemPoolManager
    MemPoolManager* instance = nullptr;
    if (isFirstProcess) {
        instance = MemPoolManager::constructInPlace(
            managerAddr,
            shmBase,
            static_cast<MemPoolConfig*>(configAddr)
        );
        
        // 初始化管理区和数据区
        if (!instance->initializeMemoryRegions(currentPtr)) {
            sem_post(s_initSemaphore);
            return false;
        }
    } else {
        // 不是第一个进程，直接获取已构造的对象
        instance = static_cast<MemPoolManager*>(managerAddr);
    }
    
    s_instance.set(instance);  // 设置相对指针
    
    sem_post(s_initSemaphore);  // 解锁
    
    return true;
}
```

### 6.2 placement new 构造

```cpp
MemPoolManager* MemPoolManager::constructInPlace(
    void* address, 
    void* sharedMemoryBase,
    const MemPoolConfig* config) noexcept
{
    // 使用 placement new 在指定地址构造对象
    MemPoolManager* instance = new (address) MemPoolManager(sharedMemoryBase, config);
    
    // 初始化进程间互斥锁
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);  // 关键！
    pthread_mutex_init(&instance->m_lock, &attr);
    pthread_mutexattr_destroy(&attr);
    
    return instance;
}
```

### 6.3 私有构造函数

```cpp
MemPoolManager::MemPoolManager(void* sharedMemoryBase, 
                               const MemPoolConfig* config) noexcept
    : m_sharedMemoryBase(sharedMemoryBase)
{
    m_config.set(config);  // 设置相对指针指向共享内存中的配置
}
```

---

## 七、多进程访问流程

### 进程 A（第一个进程）
```cpp
// 1. 准备配置
MemPoolConfig config;
config.addMemPoolEntry(1024, 100);
config.addMemPoolEntry(4096, 50);

// 2. 创建共享实例（会创建共享内存并初始化）
if (!MemPoolManager::createSharedInstance(config)) {
    ZEROCP_LOG(Error, "Failed to create shared instance");
    return;
}

// 3. 获取实例
MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();

// 4. 使用
ChunkManager* chunk = mgr->getChunk(2048);
```

### 进程 B（后续进程）
```cpp
// 1. 打开共享内存（已由进程 A 创建）
MemPoolConfig dummyConfig;  // 不会被使用
if (!MemPoolManager::createSharedInstance(dummyConfig)) {
    ZEROCP_LOG(Error, "Failed to attach to shared instance");
    return;
}

// 2. 获取实例（指向进程 A 创建的同一个对象）
MemPoolManager* mgr = MemPoolManager::getInstanceIfInitialized();

// 3. 使用（与进程 A 共享同一个 MemPoolManager）
ChunkManager* chunk = mgr->getChunk(2048);
```

---

## 八、关键注意事项

### 1. 所有成员必须支持共享内存
✅ `vector<MemPool, 16>` - 固定容量，可以
✅ `RelativePointer` - 专为共享内存设计
✅ `pthread_mutex_t` - 设置 `PTHREAD_PROCESS_SHARED`
✅ `std::atomic` - 可以跨进程
❌ `std::vector` - 动态分配，不行
❌ `std::mutex` - 不支持跨进程
❌ 普通指针 - 不同进程地址不同

### 2. 对齐要求
所有共享内存中的对象都需要适当对齐（通常 8 字节）

### 3. 构造顺序
1. 创建共享内存
2. 在共享内存中构造 MemPoolConfig
3. 在共享内存中构造 MemPoolManager
4. 初始化管理区和数据区
5. 构造各个 MemPool

### 4. 析构问题
共享内存中的对象不会自动析构，需要：
- 最后一个进程负责析构
- 或使用引用计数跟踪进程数量

### 5. 进程同步
使用命名信号量 (`sem_open`) 或共享内存中的 `pthread_mutex_t`

---

## 九、完整内存布局示例

假设配置：
- Pool 0: 1024 字节 × 100 个
- Pool 1: 4096 字节 × 50 个

```
地址偏移           大小                     内容
─────────────────────────────────────────────────────────
0x0000          ~200 bytes            MemPoolManager 对象
                                      - m_sharedMemoryBase
                                      - m_config (RelativePointer)
                                      - m_mempools (vector<MemPool, 16>)
                                      - m_chunkManagerPool (vector<MemPool, 1>)
                                      - m_lock (pthread_mutex_t)

0x00C8          ~100 bytes            MemPoolConfig 对象
                                      - m_memPoolEntries (vector<MemPoolEntry, 16>)

0x012C          X bytes               管理区
                                      - Pool 0 的 freeList (100 个索引)
                                      - Pool 1 的 freeList (50 个索引)
                                      - ChunkManager[0..149] (150 个对象)
                                      - ChunkManagerPool 的 freeList

0x012C + X      102,400 bytes         数据区 - Pool 0
                                      - Chunk[0..99]，每个 1024 字节

0x012C + X      204,800 bytes         数据区 - Pool 1
+ 102,400                             - Chunk[0..49]，每个 4096 字节
```

---

## 十、总结

| 修改项 | 原实现 | 新实现 |
|--------|--------|--------|
| **MemPoolManager 位置** | 进程堆上 (`new`) | 共享内存中 (`placement new`) |
| **m_config** | `const MemPoolConfig&` | `RelativePointer<const MemPoolConfig>` |
| **m_memPoolEntries** | `std::vector` | `ZeroCP::vector<MemPoolEntry, 16>` |
| **s_instance** | `MemPoolManager*` | `RelativePointer<MemPoolManager>` 或进程本地指针 |
| **s_mutex** | `std::mutex` | `pthread_mutex_t` (PROCESS_SHARED) 或命名信号量 |
| **初始化** | 每个进程独立 | 第一个进程初始化，后续附加 |
| **多进程** | 不支持 | ✅ 完全支持 |

通过这些修改，整个 `MemPoolManager` 可以完全在共享内存中，所有进程共享同一个实例！

