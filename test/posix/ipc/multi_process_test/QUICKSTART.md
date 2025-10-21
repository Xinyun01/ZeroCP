# 🚀 快速开始指南

这是一个 **5 分钟上手指南**，帮助你快速运行多进程 Unix Domain Socket 通信测试。

## ⚡ 超级快速开始（3 步）

```bash
# 1. 进入测试目录
cd test/posix/ipc/multi_process_test

# 2. 一键运行（自动编译和测试）
./test.sh

# 3. 查看结果 ✓
```

就这么简单！🎉

## 📋 前置要求

确保你的系统有：

- ✅ Linux 操作系统
- ✅ C++23 编译器（GCC 10+ 或 Clang 12+）
- ✅ CMake 3.16+

快速检查：

```bash
g++ --version    # 或 clang++ --version
cmake --version
```

## 🎯 使用方式

### 方式 1: 一键测试（推荐）

```bash
./test.sh         # Release 模式
./test.sh debug   # Debug 模式
```

### 方式 2: 使用 Makefile

```bash
make              # 编译并测试
make debug        # Debug 模式
make clean        # 清理
```

### 方式 3: 分步执行

```bash
# 编译
./build.sh

# 运行测试
./run_test.sh
```

### 方式 4: 手动运行（调试用）

```bash
cd build/

# 终端 1: 启动服务端
./uds_server

# 终端 2-4: 启动客户端
./uds_client1
./uds_client2
./uds_client3
```

## 📊 预期输出

成功运行后，你会看到：

```
========================================
  Test Summary
========================================
  ✓ ALL TESTS PASSED
========================================
```

## 🔍 查看日志

测试运行后，日志文件在 `build/` 目录：

```bash
cat build/server.log   # 服务端日志
cat build/client1.log  # 客户端 1 日志
cat build/client2.log  # 客户端 2 日志
cat build/client3.log  # 客户端 3 日志
```

或使用 Makefile：

```bash
make logs
```

## 🧹 清理

```bash
# 清理构建文件
make clean

# 清理所有（包括 socket 文件）
make clean-all

# 只清理 socket 文件
make clean-sockets
```

## ❓ 常见问题

### Q: 编译失败？

**A**: 检查编译器和 CMake 版本：

```bash
g++ --version   # 需要 >= 10.0
cmake --version # 需要 >= 3.16
```

### Q: 测试失败？

**A**: 先清理旧文件：

```bash
make clean-all
./test.sh
```

### Q: Socket 文件已存在？

**A**: 运行清理命令：

```bash
rm -f /tmp/uds_*.sock
```

或

```bash
make clean-sockets
```

### Q: 进程未退出？

**A**: 手动杀死：

```bash
pkill -f uds_server
pkill -f uds_client
```

或

```bash
make kill-processes
```

## 🎓 深入了解

想了解更多细节？查看完整文档：

```bash
cat README.md
```

或查看项目信息：

```bash
make info
```

## 🛠️ 可用的 Makefile 目标

```bash
make              # 编译并测试
make build        # 只编译
make test         # 只测试
make debug        # Debug 模式编译
make clean        # 清理构建
make clean-all    # 完全清理
make rebuild      # 重新编译
make logs         # 查看日志
make info         # 项目信息
make help         # 帮助信息
```

## 📞 需要帮助？

- 查看详细文档：[README.md](README.md)
- 查看配置文件：[config.hpp](config.hpp)
- 检查代码：[server.cpp](server.cpp), [client1.cpp](client1.cpp)

---

**祝测试愉快！** 🚀

