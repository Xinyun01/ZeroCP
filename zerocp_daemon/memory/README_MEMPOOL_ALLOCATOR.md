# MemPoolAllocator 使用指南

## 概述

`MemPoolAllocator` 是内存池系统的高层接口，负责整合配置、共享内存创建和内存池初始化。它提供了简洁的 API 来完成整个内存池系统的启动过程。

## 架构设计

```
MemPoolAllocator
    ├── MemPoolConfig           # 内存池配置
    ├── PosixShmProvider        # 共享内存提供者
    └── MemPoolManager          # 内存池管理器
```

### 职责划分

1. **MemPoolConfig**: 定义内存池的大小和数量配置
2. **MemPoolAllocator**: 
   - 计算所需共享内存大小
   - 创建和管理共享内存
   - 协调内存池的初始化
3. **MemPoolManager**: 管理所有内存池实例，提供内存分配/释放接口

## API 接口

### 核心接口

#### 1. `initializeSharedMemory()`

```cpp
std::expected<void*, Details::PosixSharedMemoryObjectError> 
initializeSharedMemory(const std::string& shmName) noexcept;
```

**功能**: 创建共享内存并返回基地址

**参数**:
- `shmName`: 共享内存名称（POSIX 共享内存命名规则）

**返回值**:
- 成功: 返回共享内存基地址
- 失败: 返回 `PosixSharedMemoryObjectError` 错误码

**使用场景**: 
- 必须在初始化内存池之前调用
- 自动计算所需的共享内存大小
- 使用 `PurgeAndCreate` 模式确保创建新的共享内存

**示例**:
```cpp
MemPoolAllocator allocator(config);
auto result = allocator.initializeSharedMemory("/my_shm");
if (result.has_value()) {
    void* baseAddr = result.value();
    // 使用基地址...
}
```

---

#### 2. `initializeMemPools()`

```cpp
bool initializeMemPools() noexcept;
```

**功能**: 按照配置在共享内存中布局并初始化所有内存池

**返回值**:
- `true`: 初始化成功
- `false`: 初始化失败

**前置条件**:
- 必须先调用 `initializeSharedMemory()` 成功

**内部流程**:
1. 检查共享内存是否已创建
2. 获取 `MemPoolManager` 单例
3. 调用 `MemPoolManager::initialize()` 布局内存
4. 打印所有内存池状态

**示例**:
```cpp
// 先初始化共享内存
auto result = allocator.initializeSharedMemory("/my_shm");
if (!result.has_value()) {
    return -1;
}

// 再初始化内存池
if (!allocator.initializeMemPools()) {
    std::cerr << "Failed to initialize memory pools" << std::endl;
    return -1;
}
```

---

### 查询接口

#### 3. `getBaseAddress()`

```cpp
void* getBaseAddress() const noexcept;
```

**功能**: 获取共享内存基地址

**返回值**: 基地址指针，未初始化则返回 `nullptr`

---

#### 4. `getMemorySize()`

```cpp
uint64_t getMemorySize() const noexcept;
```

**功能**: 获取共享内存总大小（字节）

**返回值**: 共享内存大小，构造时自动计算

---

#### 5. `isInitialized()`

```cpp
bool isInitialized() const noexcept;
```

**功能**: 检查分配器是否完全初始化

**返回值**: 
- `true`: 共享内存和内存池都已初始化
- `false`: 未完全初始化

**检查项**:
- 共享内存提供者已创建
- 基地址非空
- `MemPoolManager` 单例已创建
- `MemPoolManager` 已初始化

---

## 完整使用流程

### 步骤1: 创建配置

```cpp
using namespace ZeroCP::Memory;

MemPoolConfig config;
config.addMemPoolEntry(256, 1000);   // 256字节 x 1000个
config.addMemPoolEntry(1024, 500);   // 1KB x 500个
config.addMemPoolEntry(4096, 200);   // 4KB x 200个
```

### 步骤2: 创建分配器

```cpp
MemPoolAllocator allocator(config);

// 查看所需内存大小
uint64_t requiredMemory = allocator.getMemorySize();
std::cout << "Required memory: " << requiredMemory << " bytes" << std::endl;
```

### 步骤3: 初始化共享内存

```cpp
const std::string shmName = "/my_application_shm";
auto shmResult = allocator.initializeSharedMemory(shmName);

if (!shmResult.has_value()) {
    std::cerr << "Failed to initialize shared memory" << std::endl;
    return -1;
}

void* baseAddress = shmResult.value();
std::cout << "Shared memory created at: " << baseAddress << std::endl;
```

### 步骤4: 初始化内存池

```cpp
if (!allocator.initializeMemPools()) {
    std::cerr << "Failed to initialize memory pools" << std::endl;
    return -1;
}

std::cout << "Memory pools initialized successfully" << std::endl;
```

### 步骤5: 使用内存池

```cpp
// 获取内存池管理器
MemPoolManager* poolManager = MemPoolManager::getInstanceIfInitialized();

// 分配内存
ChunkManager* chunk = poolManager->getChunk(512);
if (chunk != nullptr) {
    // 使用内存...
    
    // 释放内存
    poolManager->releaseChunk(chunk);
}
```

---

## 错误处理

### 共享内存初始化失败

```cpp
auto result = allocator.initializeSharedMemory("/my_shm");
if (!result.has_value()) {
    auto error = result.error();
    switch (error) {
        case PosixSharedMemoryObjectError::INVALID_SIZE:
            std::cerr << "Invalid memory size" << std::endl;
            break;
        case PosixSharedMemoryObjectError::CREATION_FAILED:
            std::cerr << "Failed to create shared memory" << std::endl;
            break;
        case PosixSharedMemoryObjectError::MAPPING_ERROR:
            std::cerr << "Failed to map shared memory" << std::endl;
            break;
        // 处理其他错误...
    }
    return -1;
}
```

### 内存池初始化失败

```cpp
if (!allocator.initializeMemPools()) {
    // 检查日志输出以确定失败原因
    // 可能的原因：
    // 1. 共享内存未初始化
    // 2. MemPoolManager 单例未创建
    // 3. 内存布局失败
    return -1;
}
```

---

## 日志输出

分配器会自动输出详细的日志信息（使用 `ZEROCP_LOG` 宏）：

### 构造时
```
[Info] MemPoolAllocator 已初始化，计算得到总内存大小: 10485760 字节
```

### 初始化共享内存
```
[Info] 开始创建共享内存: /my_shm, 大小: 10485760 字节
[Info] 共享内存创建成功: /my_shm, 基地址: 0x7f1234567000, 段ID: 42
```

### 初始化内存池
```
[Info] 开始初始化内存池，基地址: 0x7f1234567000, 总大小: 10485760 字节
[Info] 内存池管理器初始化成功
[Info] Pool 0: chunk_size=256, count=1000, used=0
[Info] Pool 1: chunk_size=1024, count=500, used=0
[Info] Pool 2: chunk_size=4096, count=200, used=0
```

---

## 注意事项

### 1. 调用顺序

必须按照以下顺序调用：
```
构造 MemPoolAllocator
    ↓
initializeSharedMemory()
    ↓
initializeMemPools()
    ↓
使用 MemPoolManager 进行内存分配
```

### 2. 单例模式

- `MemPoolManager` 是全局单例
- 同一进程中只能有一个 `MemPoolManager` 实例
- 多个 `MemPoolAllocator` 可以共享同一个配置

### 3. 线程安全

- `initializeSharedMemory()` 和 `initializeMemPools()` 不是线程安全的
- 应在主线程初始化完成后再启动多线程
- `MemPoolManager` 的分配/释放接口是线程安全的

### 4. 内存管理

- 共享内存由 `PosixShmProvider` 管理
- 析构时自动清理（如果配置为 `PurgeAndCreate` 模式）
- 确保在进程退出前不要手动删除共享内存对象

### 5. 配置不可变

- 配置在 `MemPoolAllocator` 构造后不能修改
- 如需修改配置，必须创建新的 `MemPoolAllocator` 实例

---

## 示例程序

完整示例请参考: `examples/mempool_allocator_usage.cpp`

编译运行:
```bash
cd examples
g++ -std=c++20 mempool_allocator_usage.cpp -o mempool_demo -lrt -lpthread
./mempool_demo
```

---

## 设计优势

1. **清晰的职责分离**: 配置、共享内存、内存池分别管理
2. **错误处理友好**: 使用 `std::expected` 提供详细的错误信息
3. **易于使用**: 简单的两步初始化流程
4. **详细的日志**: 自动记录所有关键步骤
5. **类型安全**: 使用强类型而非裸指针
6. **RAII**: 资源自动管理，避免内存泄漏

---

## 相关文档

- [MemPoolConfig 使用指南](./README_MEMPOOL_CONFIG.md)
- [MemPoolManager API 文档](./README_MEMPOOL_MANAGER.md)
- [PosixShmProvider 使用指南](../sharememory/README_POSIXSHM.md)

