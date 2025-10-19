# ZeroCopy Framework Test Suite

## 📋 概述

这是 ZeroCopy 框架的统一测试目录，包含所有模块的测试程序。

## 📁 目录结构

```
test/
├── README.md                    # 本文档
├── posix/                       # POSIX 相关测试
│   └── ipc/                     # IPC（进程间通信）测试
│       ├── test_posix_sharedmemory.cpp
│       ├── CMakeLists.txt
│       ├── build_and_run_tests.sh
│       └── README_TEST.md
└── report/                      # 日志系统测试
    ├── complete_demo.cpp        # 完整功能演示
    ├── test_backend.cpp         # 后端测试
    ├── test_logstream.cpp       # 日志流测试
    ├── test_fixed_buffer.cpp    # 固定缓冲区测试
    ├── test_startup.cpp         # 启动测试
    ├── test_comprehensive.cpp   # 综合测试
    ├── test_performance.cpp     # 性能测试
    ├── CMakeLists.txt
    ├── build_and_run_tests.sh
    └── README_TEST.md
```

## 🚀 快速开始

### 运行所有测试

```bash
# 运行 POSIX IPC 测试
cd test/posix/ipc
./build_and_run_tests.sh

# 运行 Report 模块测试
cd test/report
./build_and_run_tests.sh
```

### 单独构建和运行

每个测试模块都有独立的构建系统和运行脚本，详见各自的 `README_TEST.md`。

## 📦 测试模块

### 1. POSIX IPC 测试 (`posix/ipc/`)

测试 POSIX 共享内存和内存映射功能：
- 共享内存创建和打开
- 内存映射（mmap）
- 数据读写验证
- 错误处理
- 移动语义
- 不同权限和大小

**运行方式**：
```bash
cd test/posix/ipc
./build_and_run_tests.sh
```

### 2. Report 模块测试 (`report/`)

测试异步日志系统的各项功能：

#### 测试程序列表

| 程序 | 说明 |
|------|------|
| `complete_demo` | 完整功能演示 |
| `test_backend` | 后端功能测试 |
| `test_logstream` | 日志流测试 |
| `test_fixed_buffer` | 固定缓冲区测试 |
| `test_startup` | 启动流程测试 |
| `test_comprehensive` | 综合功能测试 |
| `test_performance` | 性能基准测试 |

**运行方式**：
```bash
cd test/report
./build_and_run_tests.sh
# 然后选择要运行的测试（1-7）或运行所有测试（8）
```

## 🔧 构建要求

### 通用要求
- **编译器**：支持 C++17 的 GCC 或 Clang
- **CMake**：版本 3.10 或更高
- **操作系统**：Linux
- **线程库**：pthread

### 各模块特定要求

#### POSIX IPC
- POSIX 共享内存支持（`shm_open`, `shm_unlink`）
- 内存映射支持（`mmap`, `munmap`）

#### Report
- 无锁队列支持
- 高精度时间戳
- 文件 I/O

## 📊 测试覆盖

### POSIX IPC 测试覆盖
- ✅ 共享内存创建（ExclusiveCreate, PurgeAndCreate, OpenOrCreate）
- ✅ 共享内存打开（OpenExisting）
- ✅ 内存映射和数据访问
- ✅ 不同访问模式（ReadOnly, WriteOnly, ReadWrite）
- ✅ 不同文件权限（0600, 0700, 0777）
- ✅ 错误处理和边界条件
- ✅ 移动语义和资源管理

### Report 测试覆盖
- ✅ 基本日志记录
- ✅ 多级别日志（Debug, Info, Warn, Error, Fatal）
- ✅ 流式输出（LogStream）
- ✅ 多线程并发
- ✅ 后端队列管理
- ✅ 性能基准测试
- ✅ 启动和关闭流程
- ✅ 固定缓冲区管理

## 🐛 调试

### 启用调试模式

```bash
cd test/<module>/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### 使用 GDB

```bash
cd test/<module>/build
gdb ./<test_executable>
```

### 查看详细输出

大多数测试程序会输出详细的日志和统计信息，帮助诊断问题。

## 📚 相关文档

- [POSIX IPC 测试文档](posix/ipc/README_TEST.md)
- [Report 模块测试文档](report/README_TEST.md)
- [POSIX IPC 模块文档](../zerocp_foundationLib/posix/ipc/README.md)
- [Report 模块文档](../zerocp_foundationLib/report/README.md)

## 🎯 测试原则

1. **独立性**：每个测试模块独立构建和运行
2. **完整性**：覆盖核心功能和边界条件
3. **可读性**：测试代码清晰，输出易懂
4. **自动化**：提供脚本自动构建和运行
5. **文档化**：每个测试都有详细说明

## 🔄 持续集成

测试套件设计为可以集成到 CI/CD 流程中：

```bash
# 示例 CI 脚本
#!/bin/bash
set -e

# 测试 POSIX IPC
cd test/posix/ipc
mkdir -p build && cd build
cmake .. && make
./test_posix_sharedmemory

# 测试 Report
cd ../../../report
mkdir -p build && cd build
cmake .. && make
./test_backend
./test_logstream
# ... 其他测试
```

## 📞 问题反馈

如果测试失败或遇到问题：

1. 检查编译器版本和依赖
2. 查看测试输出的错误信息
3. 阅读相关模块的文档
4. 使用调试模式重新编译
5. 检查系统资源（内存、权限等）

---

**注意**：
- 某些测试可能需要特定的系统权限
- 性能测试结果会因系统负载而异
- 建议在干净的环境中运行测试


