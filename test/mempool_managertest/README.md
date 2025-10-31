# MemPoolManager 测试套件

## 概述

本测试套件用于验证 `MemPoolManager` 的双共享内存架构，包括单进程和多进程场景。

### 双共享内存架构

```
┌─────────────────────────────────────────┐
│ 管理区共享内存 (/zerocp_memory_management)
│  - MemPoolManager 实例本身             │
│  - freeList (MPMCLockFreeList)         │
│  - ChunkManager Pool                   │
│  - MemPool 元数据（使用 RelativePointer)│
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ 数据区共享内存 (/zerocp_memory_chunk)   │
│  - Chunk 1: [ChunkHeader][用户数据]    │
│  - Chunk 2: [ChunkHeader][用户数据]    │
│  - Chunk N: [ChunkHeader][用户数据]    │
└─────────────────────────────────────────┘
```

### 关键设计要点

1. **进程本地变量**（每个进程不同）：
   - `s_managementBaseAddress`: 管理区映射地址
   - `s_chunkBaseAddress`: 数据区映射地址
   - `s_managementMemorySize`: 管理区大小
   - `s_chunkMemorySize`: 数据区大小

2. **共享内存数据**（所有进程相同）：
   - 使用 `RelativePointer<T>` 存储相对偏移量
   - 不存储绝对指针
   - 确保多进程数据一致性

## 测试文件

### 1. test_mempool_creation.cpp

**MemPoolManager 创建验证测试** ⭐ 推荐先运行

测试内容：
- ✅ MemPoolConfig 配置创建
- ✅ 共享实例创建验证
- ✅ 内存布局验证（管理区 + 数据区）
- ✅ MemPool 配置验证
- ✅ ChunkManagerPool 验证
- ✅ 统计信息打印
- ✅ 清理和销毁验证

**特点**：
- 简单快速，专注于验证创建过程
- 适合快速检查 MemPoolManager 是否正常工作
- 包含详细的状态输出和验证

### 2. test_mempool_manager_basic.cpp

**单进程基础测试**

测试内容：
- ✅ MemPoolConfig 创建和配置
- ✅ 共享实例创建 (`createSharedInstance`)
- ✅ 内存大小计算（管理区 + 数据区）
- ✅ MemPool 访问和验证
- ✅ 基地址验证（双共享内存）
- ✅ 统计信息打印
- ✅ 共享实例销毁 (`destroySharedInstance`)

### 3. test_mempool_manager_multiprocess.cpp

**多进程共享内存测试**

测试内容：
- ✅ 父进程创建共享实例
- ✅ 子进程附加到已有实例
- ✅ 验证多进程看到相同的 MemPool 配置
- ✅ 验证 RelativePointer 在多进程间正常工作
- ✅ 验证进程本地变量可以不同（不同的映射地址）

## 快速开始

### 方式 1: 使用脚本（推荐）

#### 快速验证创建（推荐第一次运行）
```bash
# 运行 MemPoolManager 创建验证测试
./run_creation_test.sh
```

这个脚本会：
1. 清理旧的共享内存
2. 配置和编译测试程序
3. 运行创建验证测试
4. 显示详细的创建过程和状态
5. 自动清理测试环境

#### 运行所有测试
```bash
# 运行完整测试套件
./run_test.sh
```

这个脚本会：
1. 清理旧的 build 目录
2. 配置 CMake
3. 编译所有测试程序
4. 运行所有测试
5. 输出测试结果总结

### 方式 2: 手动编译和运行

```bash
# 1. 创建 build 目录
mkdir -p build
cd build

# 2. 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 3. 编译
make -j$(nproc)

# 4. 运行测试 1: 创建验证测试（推荐先运行）
./bin/test_mempool_creation

# 5. 运行测试 2: 单进程基础测试
./bin/test_mempool_manager_basic

# 6. 运行测试 3: 多进程共享内存测试
./bin/test_mempool_manager_multiprocess
```

### 方式 3: 使用 CTest

```bash
cd build
ctest --verbose
```

## 测试输出示例

### 基础单进程测试

```
======================================================================
MemPoolManager 基础单进程测试
测试双共享内存架构（管理区 + 数据区）
======================================================================

[TEST 1] MemPoolConfig 创建和配置
  配置项数量: 3
  Pool[0]: chunkSize=128, count=100
  Pool[1]: chunkSize=1024, count=50
  Pool[2]: chunkSize=4096, count=25
✓ PASS - MemPoolConfig 创建和配置 (3个内存池配置成功)

[TEST 2] 共享实例创建
  共享实例创建成功
  实例地址: 0x7f...
✓ PASS - 共享实例创建 (实例创建并获取成功)

[TEST 3] 内存大小计算（管理区 + 数据区）
  管理区大小:      12800 字节 (12.5 KB)
  数据区大小:     153600 字节 (150.0 KB)
  ──────────────────────────────
  总内存大小:     166400 字节 (0.158691 MB)
✓ PASS - 内存大小计算 (Total=162 KB)

...

🎉 所有测试通过！
```

### 多进程共享内存测试

```
======================================================================
MemPoolManager 多进程共享内存测试
测试架构：双共享内存（管理区 + 数据区）
测试重点：进程本地变量 vs 相对偏移量
======================================================================

[父进程] PID=12345
[父进程] 创建 MemPoolConfig，3个内存池
[父进程] 共享实例创建成功: 0x7f1234567000
[父进程] 内存布局:
  管理区大小: 12800 字节
  数据区大小: 153600 字节
  总内存大小: 166400 字节 (0.158691 MB)
...

[子进程] PID=12346
[子进程] 成功附加到共享实例: 0x7e9876543000  ← 注意：地址可能不同！
[子进程] 内存大小:
  管理区: 12800 字节
  数据区: 153600 字节
  总计:   166400 字节
[子进程] ✓ 所有验证通过！

[父进程] ✓ 所有验证通过！

🎉 多进程测试全部通过！
```

## 验证要点

### ✅ 应该验证的

1. **内存大小一致性**
   - 管理区大小计算正确
   - 数据区大小计算正确
   - 总大小 = 管理区 + 数据区

2. **MemPool 配置正确**
   - MemPool 数量正确
   - 每个 MemPool 的 chunkSize 和 chunkCount 正确

3. **多进程数据一致性**
   - 父子进程看到相同的 MemPool 配置
   - 父子进程看到相同的内存大小

4. **进程本地变量独立性**
   - 不同进程的映射地址可以不同
   - 但共享内存中的数据是相同的

### ⚠️ 注意事项

1. **绝对指针禁止**
   - 永远不要在共享内存中存储绝对指针
   - 只能使用 `RelativePointer<T>`

2. **基地址是进程本地的**
   - `s_managementBaseAddress` 在不同进程中可能不同
   - `s_chunkBaseAddress` 在不同进程中可能不同
   - 这是正常的，也是预期的

3. **共享内存清理**
   - 测试脚本会自动清理
   - 如果测试崩溃，可能需要手动清理：
     ```bash
     # 查看共享内存
     ls -lh /dev/shm/
     
     # 手动删除
     rm /dev/shm/zerocp_memory_management
     rm /dev/shm/zerocp_memory_chunk
```

## 故障排查

### 问题 1: "Failed to create shared instance"

**可能原因**：
- 权限不足
- 共享内存已存在（上次测试未清理）

**解决方案**：
```bash
# 清理共享内存
rm -f /dev/shm/zerocp_memory_*
rm -f /dev/shm/zerocp_init_sem

# 重新运行测试
./run_test.sh
```

### 问题 2: 子进程无法附加

**可能原因**：
- 父进程还未创建共享内存
- 信号量未正确初始化

**解决方案**：
- 检查日志输出
- 增加子进程的 sleep 时间

### 问题 3: 编译错误

**可能原因**：
- 缺少依赖库
- C++ 标准不支持

**解决方案**：
   ```bash
# 确保使用 C++23
g++ --version

# 确保安装了 rt 库
sudo apt-get install libc6-dev
   ```

## 开发指南

### 添加新测试

1. 在 `test_mempool_manager_basic.cpp` 中添加单进程测试
2. 在 `test_mempool_manager_multiprocess.cpp` 中添加多进程测试
3. 更新 README

### 测试最佳实践

1. **每个测试独立**：不依赖其他测试的状态
2. **清理资源**：测试结束后清理共享内存
3. **详细日志**：使用颜色输出，便于定位问题
4. **验证假设**：使用 `assert` 验证关键假设

## 相关文档

- [共享内存基地址设计说明](../../docs/Shared_Memory_Base_Address_Design.md)
- [MemPoolManager API 文档](../../zerocp_daemon/memory/include/mempool_manager.hpp)
- [RelativePointer 实现](../../zerocp_daemon/memory/include/relative_pointer.hpp)

## 许可证

Copyright © 2025 ZeroCopy Framework Team
