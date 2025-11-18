# Iceoryx-Style Distributed Construction Pattern

## 概述

本文档详细说明了在 ZeroCP 框架中实现的 iceoryx 风格分布式构造模式，用于在 POSIX 共享内存中创建和管理组件。

## 核心设计理念

参考 iceoryx::roudi::MemoryManager 的设计，实现了三阶段分布式构造：

1. **预分配内存** (Reserve Memory) - 使用 `std::aligned_storage_t` 预留对齐的内存空间
2. **分步构造** (Distributed Construction) - 使用 placement new 分阶段构造各个组件
3. **显式析构** (Explicit Destruction) - 按照 LIFO 顺序显式调用析构函数

## 架构设计

### 1. DirouteComponents - 内存布局容器

```cpp
struct DirouteComponents
{
    // Step 1: 使用 aligned_storage 预留内存（未构造）
    using HeartbeatBlockStorage = std::aligned_storage_t<
        sizeof(zerocp::memory::HeartbeatBlock),
        alignof(zerocp::memory::HeartbeatBlock)>;
    HeartbeatBlockStorage m_heartbeatBlockStorage;
    
    // Step 2: 构造状态追踪
    bool m_heartbeatBlockConstructed{false};
    
    // Step 3: 分布式构造方法
    zerocp::memory::HeartbeatBlock& constructHeartbeatBlock() noexcept
    {
        if (!m_heartbeatBlockConstructed)
        {
            // Placement new: 在预留内存中构造对象
            new (&m_heartbeatBlockStorage) zerocp::memory::HeartbeatBlock();
            m_heartbeatBlockConstructed = true;
        }
        return *reinterpret_cast<zerocp::memory::HeartbeatBlock*>(&m_heartbeatBlockStorage);
    }
    
    // Step 4: 显式析构 (LIFO)
    ~DirouteComponents() noexcept
    {
        if (m_heartbeatBlockConstructed)
        {
            reinterpret_cast<zerocp::memory::HeartbeatBlock*>(
                &m_heartbeatBlockStorage)->~HeartbeatBlock();
            m_heartbeatBlockConstructed = false;
        }
    }
};
```

**关键特性：**
- ✅ 内存预留与对象构造分离
- ✅ 支持分阶段初始化
- ✅ 显式生命周期管理
- ✅ LIFO 析构顺序保证

### 2. DirouteMemoryManager - 静态构造管理器

参考 iceoryx::roudi::MemoryManager，提供静态工厂方法来创建和初始化共享内存池。

```cpp
class DirouteMemoryManager
{
public:
    // 静态工厂方法 - iceoryx 模式
    [[nodiscard]] static std::expected<DirouteMemoryManager, MemoryManagerError>
    createMemoryPool(const Config& config = Config{}) noexcept;
    
private:
    // Phase 1: 创建 POSIX 共享内存
    static std::expected<PosixSharedMemoryObject, MemoryManagerError>
    createSharedMemory(const Config& config) noexcept;
    
    // Phase 2: 在共享内存中构造 DirouteComponents
    static std::expected<DirouteComponents*, MemoryManagerError>
    constructComponents(void* baseAddress) noexcept;
    
    // Phase 3: 分布式构造 HeartbeatBlock
    static std::expected<void, MemoryManagerError>
    constructHeartbeatBlock(DirouteComponents* components) noexcept;
};
```

## 三阶段构造流程

### Phase 1: 创建共享内存

```cpp
auto shmResult = PosixSharedMemoryObjectBuilder()
    .name("/zerocp_diroute_components")
    .memorySize(sizeof(DirouteComponents))
    .accessMode(AccessMode::ReadWrite)
    .openMode(OpenMode::PurgeAndCreate)
    .permissions(Perms::OwnerAll | Perms::GroupRead | Perms::GroupWrite)
    .create();

void* baseAddress = shmResult->getBaseAddress();
```

**作用：**
- 创建或打开 POSIX 共享内存段
- 映射到进程地址空间
- 获取基地址用于后续构造

### Phase 2: 构造 DirouteComponents

```cpp
// 使用 placement new 在共享内存中构造
DirouteComponents* components = new (baseAddress) DirouteComponents();
```

**注意：**
- 此时只构造了 DirouteComponents 结构体本身
- 内部组件（HeartbeatBlock）仅仅是预留的内存（aligned_storage）
- **尚未真正创建 HeartbeatBlock 对象**

### Phase 3: 分布式构造组件

```cpp
// 显式构造 HeartbeatBlock
auto& heartbeatBlock = components->constructHeartbeatBlock();
```

**作用：**
- 使用 placement new 在预留的 aligned_storage 中构造 HeartbeatBlock
- 初始化内部的 FixedPositionContainer 和原子变量
- 标记组件为已构造状态

## 使用示例

### 守护进程 (Daemon)

```cpp
#include "zerocp_daemon/diroute/diroute_memory_manager.hpp"

int main()
{
    // 一键创建和初始化共享内存池（iceoryx 静态构造模式）
    auto memoryManagerResult = DirouteMemoryManager::createMemoryPool();
    
    if (!memoryManagerResult)
    {
        std::cerr << "Failed to create memory pool\n";
        return EXIT_FAILURE;
    }
    
    auto memoryManager = std::move(*memoryManagerResult);
    
    // 访问 HeartbeatBlock
    auto& heartbeatBlock = memoryManager.getHeartbeatBlock();
    
    // 使用 heartbeat
    auto slot = heartbeatBlock.allocate();
    slot->touch();
    heartbeatBlock.release(slot);
    
    // RAII: memoryManager 析构时自动清理
    return 0;
}
```

**内部执行流程：**
1. `createMemoryPool()` 调用 `createSharedMemory()` → 创建共享内存
2. 调用 `constructComponents()` → placement new DirouteComponents
3. 调用 `constructHeartbeatBlock()` → placement new HeartbeatBlock
4. 返回初始化完成的 `DirouteMemoryManager`

### 应用进程 (Application)

```cpp
#include "zerocp_daemon/diroute/diroute_components.hpp"
#include "zerocp_foundationLib/posix/memory/include/posix_sharedmemory_object.hpp"

int main()
{
    // 打开已存在的共享内存（守护进程已创建）
    auto shmResult = PosixSharedMemoryObjectBuilder()
        .name("/zerocp_diroute_components")
        .memorySize(sizeof(DirouteComponents))
        .accessMode(AccessMode::ReadWrite)
        .openMode(OpenMode::OpenExisting)  // 注意：只打开，不创建
        .create();
    
    if (!shmResult)
    {
        std::cerr << "Failed to open shared memory. Is daemon running?\n";
        return EXIT_FAILURE;
    }
    
    // 获取 DirouteComponents 指针（已由守护进程构造）
    void* baseAddress = shmResult->getBaseAddress();
    DirouteComponents* components = 
        reinterpret_cast<DirouteComponents*>(baseAddress);
    
    // 验证组件已构造
    if (!components->isHeartbeatBlockConstructed())
    {
        std::cerr << "HeartbeatBlock not constructed by daemon!\n";
        return EXIT_FAILURE;
    }
    
    // 访问 HeartbeatBlock
    auto& heartbeatBlock = components->heartbeatBlock();
    
    // 应用写入心跳
    auto slot = heartbeatBlock.allocate();
    slot->touch();  // 写入时间戳到共享内存
    heartbeatBlock.release(slot);
    
    // 重要：应用进程不调用析构函数（守护进程负责销毁）
    return 0;
}
```

## 与 iceoryx 的对比

| 特性 | iceoryx | ZeroCP 实现 |
|------|---------|------------|
| **内存预留** | `RouDiMemoryInterface::MemoryBlock` | `std::aligned_storage_t` |
| **静态构造** | `MemoryManager::createAndAnnounceMemory()` | `DirouteMemoryManager::createMemoryPool()` |
| **分布式构造** | `mempool::MemPool::ctor()` | `DirouteComponents::constructHeartbeatBlock()` |
| **构造状态** | `ConstructionState` enum | `bool m_heartbeatBlockConstructed` |
| **错误处理** | `expected<T, E>` | `std::expected<T, E>` (C++23) |
| **RAII** | `RouDiMemoryManager` 析构 | `DirouteMemoryManager` 析构 |

## 优势

### 1. 内存布局确定性
- 使用 `aligned_storage_t` 保证内存对齐
- 对象地址在构造前已知
- 适合多进程共享场景

### 2. 分阶段初始化
- 可以在构造对象前完成内存分配和验证
- 支持延迟初始化
- 构造失败时易于回滚

### 3. 生命周期可控
- 显式构造和析构调用
- LIFO 析构顺序保证
- 避免悬挂指针

### 4. 类型安全
- 编译期类型检查
- 避免 `reinterpret_cast` 误用
- 构造状态追踪

## 注意事项

### ⚠️ 应用进程的析构问题

**错误做法：**
```cpp
// ❌ 应用进程不应该调用析构函数
components->~DirouteComponents();  // 错误！会破坏守护进程的数据
```

**正确做法：**
```cpp
// ✅ 应用进程只访问，不销毁
// 让共享内存自动 unmap
return 0;
```

### ⚠️ 构造顺序依赖

如果将来添加更多组件，必须按照正确的顺序构造：

```cpp
// 正确的构造顺序
auto& heartbeatBlock = components->constructHeartbeatBlock();
auto& memPoolMgr = components->constructMemPoolManager();
auto& portMgr = components->constructPortManager();

// 析构顺序自动为 LIFO：
// portMgr → memPoolMgr → heartbeatBlock
```

### ⚠️ 多进程同步

当前实现未处理多进程同时启动的竞争问题。生产环境需要：
- 使用命名信号量同步
- 或使用文件锁
- 或保证守护进程先启动

## 扩展性

### 添加新组件

按照相同模式添加新组件：

```cpp
struct DirouteComponents
{
    // 1. 添加 aligned_storage
    using NewComponentStorage = std::aligned_storage_t<
        sizeof(NewComponent), alignof(NewComponent)>;
    NewComponentStorage m_newComponentStorage;
    
    // 2. 添加构造状态标志
    bool m_newComponentConstructed{false};
    
    // 3. 添加构造方法
    NewComponent& constructNewComponent() noexcept
    {
        if (!m_newComponentConstructed)
        {
            new (&m_newComponentStorage) NewComponent();
            m_newComponentConstructed = true;
        }
        return *reinterpret_cast<NewComponent*>(&m_newComponentStorage);
    }
    
    // 4. 在析构函数中添加清理（LIFO 顺序）
    ~DirouteComponents() noexcept
    {
        if (m_newComponentConstructed)
        {
            reinterpret_cast<NewComponent*>(
                &m_newComponentStorage)->~NewComponent();
        }
        // ... 其他组件的析构
    }
};
```

## 总结

本实现成功将 iceoryx 的分布式构造模式应用到 ZeroCP 框架中：

✅ **静态构造** - `DirouteMemoryManager::createMemoryPool()`  
✅ **内存预留** - `std::aligned_storage_t` 预分配  
✅ **分步初始化** - `constructHeartbeatBlock()` 分布式构造  
✅ **RAII 管理** - 自动析构和清理  
✅ **共享内存** - 守护进程和应用进程都可以读写  

这为后续扩展更多组件（MemPoolManager、PortManager 等）奠定了坚实的基础。
