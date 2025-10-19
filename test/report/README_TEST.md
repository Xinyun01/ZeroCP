# ZeroCopy Report Module Test Suite

## 📋 概述

这是 ZeroCopy 异步日志系统（Report 模块）的测试套件，包含多个测试程序用于验证日志系统的各项功能。

## 📁 目录结构

```
test/report/
├── CMakeLists.txt              # CMake 构建配置
├── build_and_run_tests.sh      # 构建和运行脚本
├── README_TEST.md              # 本文档
├── complete_demo.cpp           # 完整功能演示
├── test_backend.cpp            # 后端功能测试
├── test_logstream.cpp          # 日志流测试
├── test_fixed_buffer.cpp       # 固定缓冲区测试
├── test_startup.cpp            # 启动测试
├── test_comprehensive.cpp      # 综合测试
└── test_performance.cpp        # 性能基准测试
```

## 🚀 快速开始

### 构建和运行所有测试

```bash
cd test/report
chmod +x build_and_run_tests.sh
./build_and_run_tests.sh
```

脚本会提示你选择要运行的测试：
- 选项 1-7: 运行单个测试
- 选项 8: 运行所有测试
- 选项 0: 仅构建不运行

### 手动构建

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### 运行单个测试

```bash
cd build
./complete_demo          # 完整演示
./test_backend           # 后端测试
./test_logstream         # 日志流测试
./test_fixed_buffer      # 固定缓冲区测试
./test_startup           # 启动测试
./test_comprehensive     # 综合测试
./test_performance       # 性能测试
```

## 📝 测试说明

### 1. complete_demo - 完整功能演示
展示日志系统的完整功能，包括：
- 基本日志记录
- 不同日志级别
- 多线程日志
- 性能测试

### 2. test_backend - 后端功能测试
测试日志后端的核心功能：
- 后端初始化和关闭
- 消息队列操作
- 线程安全性
- 刷新机制

### 3. test_logstream - 日志流测试
测试 LogStream 的流式输出：
- 各种数据类型输出
- 格式化输出
- 缓冲区管理
- 流操作符重载

### 4. test_fixed_buffer - 固定缓冲区测试
测试固定大小缓冲区：
- 缓冲区写入
- 边界条件
- 溢出处理
- 性能特性

### 5. test_startup - 启动测试
测试日志系统的启动和初始化：
- 初始化流程
- 配置加载
- 资源分配
- 错误处理

### 6. test_comprehensive - 综合测试
全面的功能测试：
- 多线程并发
- 大量日志输出
- 压力测试
- 边界条件

### 7. test_performance - 性能基准测试
性能基准测试：
- 吞吐量测试
- 延迟测试
- 多线程性能
- 资源消耗

## 🔧 依赖关系

测试程序依赖以下源文件：
- `zerocp_foundationLib/report/source/lockfree_ringbuffer.cpp`
- `zerocp_foundationLib/report/source/logging.cpp`
- `zerocp_foundationLib/report/source/logstream.cpp`
- `zerocp_foundationLib/report/source/log_backend.cpp`

头文件目录：
- `zerocp_foundationLib/report/include/`

## 📊 测试结果

测试运行后会输出详细的测试结果，包括：
- ✅ 通过的测试
- ❌ 失败的测试
- 性能指标
- 错误信息

## 🐛 调试

如果测试失败，可以：

1. **查看编译错误**：检查 CMake 和 make 的输出
2. **启用调试模式**：
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make
   ```
3. **使用 GDB 调试**：
   ```bash
   gdb ./test_backend
   ```
4. **查看日志输出**：测试程序会输出详细的日志信息

## 📚 相关文档

- [Report 模块 README](../../zerocp_foundationLib/report/README.md)
- [死锁分析](../../zerocp_foundationLib/report/DEADLOCK_ANALYSIS.md)
- [STL 使用分析](../../zerocp_foundationLib/report/STL_USAGE_ANALYSIS.md)

## 🔄 与原测试的关系

这些测试从 `zerocp_foundationLib/report/example/` 迁移而来，保持了原有的功能，但：
- 统一放在 `test/` 目录下
- 使用独立的构建系统
- 添加了更友好的运行脚本
- 与其他模块测试（如 IPC）保持一致的结构

## ⚙️ 系统要求

- **编译器**：支持 C++17 的 GCC 或 Clang
- **CMake**：版本 3.10 或更高
- **操作系统**：Linux（使用 pthread）
- **内存**：建议至少 1GB 可用内存

## 📞 问题反馈

如果遇到问题，请检查：
1. 编译器版本是否支持 C++17
2. pthread 库是否正确链接
3. 源文件路径是否正确
4. 是否有足够的权限和资源

---

**注意**：这些测试程序会创建日志文件，请确保有足够的磁盘空间。

