# PosixSharedMemory 测试程序

## 概述

`test_posix_sharedmemory.cpp` 是一个全面的测试程序，用于测试 `PosixSharedMemory` 类的各种功能。

**注意**：本测试程序位于 `posix/ipc/test/` 目录下，这是专门用于单元测试和集成测试的目录。

## 测试用例

### 1. 创建新的共享内存 (testCase1_CreateNew)
- 测试使用 `PurgeAndCreate` 模式创建新的共享内存
- 验证句柄、所有权和内存大小

### 2. 打开已存在的共享内存 (testCase2_OpenExisting)
- 先创建一个共享内存，然后用 `OpenExisting` 模式打开它
- 验证打开已存在的共享内存不拥有所有权

### 3. OpenOrCreate 模式 (testCase3_OpenOrCreate)
- 测试 `OpenOrCreate` 模式的行为
- 第一次调用应该创建，第二次调用应该打开已存在的

### 4. 内存映射和数据读写 (testCase4_MemoryMapAndReadWrite)
- 创建共享内存并映射到进程地址空间
- 写入测试数据并读取验证

### 5. 不同访问模式 (testCase5_AccessModes)
- 测试只读、只写和读写访问模式
- 注意：只读模式不能用于创建新的共享内存

### 6. 不同大小的共享内存 (testCase6_DifferentSizes)
- 测试创建不同大小的共享内存（1KB 到 1MB）
- 验证实际分配的大小

### 7. 错误处理 - 空名称 (testCase7_ErrorHandling_EmptyName)
- 测试空名称的错误处理
- 应该返回 `EMPTY_NAME` 错误

### 8. 错误处理 - 已存在 (testCase8_ErrorHandling_AlreadyExists)
- 测试 `ExclusiveCreate` 模式下尝试创建已存在的共享内存
- 应该返回 `DOES_EXIST` 错误

### 9. 移动语义 (testCase9_MoveSemantics)
- 测试移动构造函数
- 验证移动后句柄正确转移

### 10. 不同文件权限 (testCase10_FilePermissions)
- 测试不同的文件权限设置
- 包括 `OwnerAll`、`OwnerRead|OwnerWrite` 和 `All`

## 编译和运行

### 方法 1: 使用提供的脚本
```bash
./build_and_run_tests.sh
```

### 方法 2: 手动编译
```bash
mkdir -p build
cd build
cmake .. -DCMAKE_CXX_STANDARD=23
make test_posix_sharedmemory
./test_posix_sharedmemory
```

### 方法 3: 使用 g++ 直接编译
```bash
g++ -std=c++23 \
    -I../include \
    -I../../posixcall/include \
    -I../../../filesystem/include \
    -I../../../report/include \
    -I../../../staticstl/include \
    -I../../../design \
    test_posix_sharedmemory.cpp \
    ../source/posix_sharedmemory.cpp \
    ../source/posix_memorymap.cpp \
    ../../../report/source/logging.cpp \
    ../../../report/source/logstream.cpp \
    ../../../report/source/log_backend.cpp \
    ../../../report/source/lockfree_ringbuffer.cpp \
    -o test_posix_sharedmemory \
    -lrt -pthread
```

## 预期输出

成功运行时，你应该看到类似以下的输出：

```
==========================================
  PosixSharedMemory Test Suite
==========================================

=== Test Case 1: Create new shared memory ===
✅ Successfully created shared memory
   Handle: 3
   Has ownership: Yes
   Memory size: 4096 bytes

=== Test Case 2: Open existing shared memory ===
✅ Created shared memory with 8192 bytes
✅ Successfully opened existing shared memory
   Handle: 4
   Has ownership: No
   Memory size: 8192 bytes

...

==========================================
  All tests completed!
==========================================
```

## 注意事项

1. **权限要求**：运行测试需要在 `/dev/shm` 目录下创建和删除文件的权限
2. **清理**：测试程序会自动清理创建的共享内存对象
3. **日志输出**：某些错误日志是预期的（如测试错误处理时）
4. **线程安全**：测试程序需要链接 pthread 库

## 依赖项

- C++23 编译器
- POSIX 共享内存支持 (`librt`)
- pthread 库
- ZeroCP logging 模块

## 故障排除

### 编译错误
- 确保使用支持 C++23 的编译器（GCC 13+ 或 Clang 16+）
- 检查所有头文件路径是否正确

### 运行时错误
- 检查 `/dev/shm` 目录的权限
- 确保有足够的共享内存空间
- 使用 `ipcs -m` 查看现有的共享内存段
- 使用 `ipcrm -M <key>` 清理残留的共享内存

## 相关文件

- `posix_sharedmemory.hpp` - PosixSharedMemory 类头文件
- `posix_sharedmemory.cpp` - PosixSharedMemory 类实现
- `posix_memorymap.hpp` - PosixMemoryMap 类头文件
- `posix_memorymap.cpp` - PosixMemoryMap 类实现

