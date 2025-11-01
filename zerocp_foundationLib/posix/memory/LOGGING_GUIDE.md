# RelativePointer 日志使用指南

## 概述

`RelativePointer` 现在集成了完整的日志系统，可以帮助您调试共享内存池的注册、指针解析等关键操作。

## 日志级别

```cpp
enum class LogLevel : uint8_t
{
    Off = 0,      // 关闭日志
    Fatal = 1,    // 致命错误（程序无法继续）
    Error = 2,    // 错误（严重但可恢复）
    Warn = 3,     // 警告（非预期但不严重）
    Info = 4,     // 信息（日常用户关心的）
    Debug = 5,    // 调试（开发者关心的）
    Trace = 6     // 追踪（详细调试信息）
};
```

## RelativePointer 日志记录的操作

### 1. **池注册 (Debug 级别)**

当调用 `registerPool()` 时：
```
[Debug] Registered shared memory pool: ID=1001, BaseAddress=0x7f1234000, TotalPools=1
```

### 2. **池取消注册 (Debug/Warn 级别)**

成功取消注册：
```
[Debug] Unregistering shared memory pool: ID=1001, BaseAddress=0x7f1234000
[Debug] Pool unregistered. Remaining pools: 0
```

尝试取消不存在的池：
```
[Warn] Attempted to unregister non-existent pool: ID=9999
```

### 3. **获取基地址 (Trace/Error 级别)**

成功获取：
```
[Trace] Retrieved base address for pool ID=1001, BaseAddress=0x7f1234000
```

池未找到（严重错误）：
```
[Error] Pool not found in registry: ID=1001. This may cause RelativePointer resolution to fail!
```

### 4. **计算偏移量 (Trace/Error 级别)**

成功计算：
```
[Trace] Computed offset: PoolID=1001, Ptr=0x7f1234100, Base=0x7f1234000, Offset=256
```

池未注册：
```
[Error] Cannot compute offset: Pool ID=1001 not found in registry. Ptr=0x7f1234100
```

指针无效（在段基地址之前）：
```
[Error] Invalid pointer: Ptr=0x7f1233000 is before pool base=0x7f1234000 (PoolID=1001)
```

### 5. **解析指针 (Trace/Error 级别)**

成功解析：
```
[Trace] Resolved RelativePointer: PoolID=1001, Offset=256, Base=0x7f1234000, Result=0x7f1234100
```

池未注册：
```
[Error] Cannot resolve RelativePointer: Pool ID=1001 not registered. Offset=256. Returning nullptr.
```

## 使用示例

### 启用调试日志

```cpp
#include "zerocp_foundationLib/report/include/logging.hpp"

int main()
{
    // 设置日志级别为 Debug（可以看到池注册信息）
    ZeroCP::Log::Log_Manager::getInstance().setLogLevel(ZeroCP::Log::LogLevel::Debug);
    
    // 或者设置为 Trace（最详细，可以看到每次指针解析）
    // ZeroCP::Log::Log_Manager::getInstance().setLogLevel(ZeroCP::Log::LogLevel::Trace);
    
    // 创建共享内存
    PosixShmProvider shmProvider(...);
    auto result = shmProvider.createMemory();
    // 输出: [Debug] Registered shared memory segment: ID=..., BaseAddress=..., TotalSegments=...
    
    // 使用 RelativePointer
    RelativePointer<MyStruct> relPtr(ptr, poolId);
    // 输出: [Trace] Computed offset: PoolID=..., Ptr=..., Base=..., Offset=...
    
    MyStruct* actualPtr = relPtr.get();
    // 输出: [Trace] Retrieved base address for pool ID=...
    // 输出: [Trace] Resolved RelativePointer: PoolID=..., Offset=..., Base=..., Result=...
    
    return 0;
}
```

### 生产环境配置

```cpp
// 生产环境：只记录 Error 及以上级别
ZeroCP::Log::Log_Manager::getInstance().setLogLevel(ZeroCP::Log::LogLevel::Error);
```

### 性能分析配置

```cpp
// 开发/调试：记录所有操作
ZeroCP::Log::Log_Manager::getInstance().setLogLevel(ZeroCP::Log::LogLevel::Trace);
```

## 日志输出示例

### 完整的写入-读取流程日志

```
=== Writer Process ===
[Debug] Registered shared memory segment: ID=12345, BaseAddress=0x7f8a00000000, TotalSegments=1
[Trace] Computed offset: PoolID=12345, Ptr=0x7f8a00000100, Base=0x7f8a00000000, Offset=256
[Trace] Computed offset: PoolID=12345, Ptr=0x7f8a00000200, Base=0x7f8a00000000, Offset=512
[Trace] Retrieved base address for pool ID=12345, BaseAddress=0x7f8a00000000
[Trace] Resolved RelativePointer: PoolID=12345, Offset=256, Base=0x7f8a00000000, Result=0x7f8a00000100

=== Reader Process (不同地址空间) ===
[Debug] Registered shared memory segment: ID=12345, BaseAddress=0x7fb200000000, TotalSegments=1
[Trace] Retrieved base address for pool ID=12345, BaseAddress=0x7fb200000000
[Trace] Resolved RelativePointer: PoolID=12345, Offset=256, Base=0x7fb200000000, Result=0x7fb200000100
[Trace] Retrieved base address for pool ID=12345, BaseAddress=0x7fb200000000
[Trace] Resolved RelativePointer: PoolID=12345, Offset=512, Base=0x7fb200000000, Result=0x7fb200000200
```

**注意**：虽然两个进程的 `BaseAddress` 不同，但通过 `RelativePointer`，相同的偏移量被正确解析到各自进程的正确虚拟地址。

## 常见错误诊断

### 错误 1: 池未注册

```
[Error] Segment not found in registry: ID=1001. This may cause RelativePointer resolution to fail!
```

**原因**：
- 忘记调用 `PosixShmProvider::createMemory()`
- 池 ID 不匹配

**解决方法**：
```cpp
// 确保先创建/映射共享内存
auto result = shmProvider.createMemory();
// 这会自动注册段
```

### 错误 2: 指针无效

```
[Error] Invalid pointer: Ptr=0x7f1233000 is before pool base=0x7f1234000 (PoolID=1001)
```

**原因**：
- 指针不属于该共享内存池
- 使用了错误的池 ID

**解决方法**：
```cpp
// 确保指针在池范围内
void* base = shmProvider.createMemory().value();
MyStruct* ptr = static_cast<MyStruct*>(base) + offset;  // 正确
// 不要使用不属于该池的指针
```

### 错误 3: 跨进程解析失败

```
[Error] Cannot resolve RelativePointer: Pool ID=1001 not registered. Offset=256. Returning nullptr.
```

**原因**：
- 读取进程未映射该共享内存池

**解决方法**：
```cpp
// 在读取进程中也要映射相同的共享内存
PosixShmProvider shmProvider("/same_shm_name", ...);
shmProvider.createMemory();  // 自动注册段
```

## 性能考虑

- **Trace 级别**：高频调用，建议仅在调试时使用
- **Debug 级别**：池注册/取消注册，低频操作，性能影响小
- **Error/Warn 级别**：异常情况，生产环境推荐
- **生产环境推荐设置**：`LogLevel::Error` 或 `LogLevel::Warn`

## 与示例程序集成

修改 `relative_pointer_example.cpp`：

```cpp
int main(int argc, char* argv[])
{
    // 启用详细日志
    ZeroCP::Log::Log_Manager::getInstance().setLogLevel(
        ZeroCP::Log::LogLevel::Debug  // 或 Trace
    );
    
    // ... 其余代码保持不变
}
```

编译并运行：
```bash
cd examples
./build_relative_pointer_example.sh
./build/relative_pointer_example writer
```

您将看到完整的池注册和指针操作日志！

## 总结

通过日志系统，您可以：
1. ✅ 跟踪共享内存池的注册和生命周期
2. ✅ 调试 RelativePointer 的偏移量计算
3. ✅ 诊断跨进程指针解析问题
4. ✅ 验证多池场景下的正确性
5. ✅ 快速定位内存访问错误

**建议**：开发阶段使用 `Debug` 或 `Trace` 级别，生产环境使用 `Error` 或 `Warn` 级别。

