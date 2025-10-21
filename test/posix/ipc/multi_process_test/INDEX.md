# 📚 项目文件索引

快速找到你需要的文件！

## 🎯 我想...

### 快速开始测试
👉 **[QUICKSTART.md](QUICKSTART.md)** - 5 分钟快速上手

```bash
./test.sh
```

### 深入了解项目
👉 **[README.md](README.md)** - 完整的技术文档

### 查看创建内容
👉 **[TEST_SUMMARY.md](TEST_SUMMARY.md)** - 测试框架总结

---

## 📁 文件分类

### 📜 源代码（5 个文件）
| 文件 | 说明 | 大小 |
|------|------|------|
| `config.hpp` | 共享配置（socket 路径、常量） | 2.4K |
| `server.cpp` | 服务端实现 | 5.9K |
| `client1.cpp` | 客户端 1 实现 | 6.5K |
| `client2.cpp` | 客户端 2 实现 | 6.6K |
| `client3.cpp` | 客户端 3 实现 | 6.6K |

### 🔧 构建配置（2 个文件）
| 文件 | 说明 | 大小 |
|------|------|------|
| `CMakeLists.txt` | CMake 构建配置 | 4.1K |
| `Makefile` | Make 快捷命令 | 7.2K |

### 🚀 测试脚本（3 个文件）
| 文件 | 说明 | 用法 | 大小 |
|------|------|------|------|
| `test.sh` ⭐ | 一键测试（推荐） | `./test.sh` | 3.6K |
| `build.sh` | 编译脚本 | `./build.sh [debug\|release]` | 5.0K |
| `run_test.sh` | 运行测试 | `./run_test.sh` | 7.7K |

### 📖 文档（4 个文件）
| 文件 | 说明 | 适合人群 | 大小 |
|------|------|----------|------|
| `QUICKSTART.md` ⭐ | 快速开始 | 新手 | 3.1K |
| `README.md` | 完整文档 | 深入学习 | 8.6K |
| `TEST_SUMMARY.md` | 测试总结 | 查看成果 | 6.3K |
| `INDEX.md` | 本文件 | 快速导航 | - |

### ⚙️ 配置文件（1 个文件）
| 文件 | 说明 |
|------|------|
| `.gitignore` | Git 忽略配置 |

---

## 🎯 常见任务快速指南

### 任务 1: 首次运行测试
```bash
# 方法 1: 一键运行（最简单）
./test.sh

# 方法 2: 使用 Makefile
make
```

### 任务 2: 修改配置
编辑 `config.hpp` 文件，然后重新编译：
```bash
./build.sh
```

### 任务 3: 查看测试日志
```bash
# 方法 1: 手动查看
cat build/server.log
cat build/client1.log

# 方法 2: 使用 Makefile
make logs
```

### 任务 4: 清理项目
```bash
# 清理构建文件
make clean

# 完全清理（包括 socket 文件）
make clean-all
```

### 任务 5: Debug 模式编译
```bash
# 方法 1: 直接编译
./build.sh debug

# 方法 2: 一键测试
./test.sh debug

# 方法 3: 使用 Makefile
make debug
```

### 任务 6: 手动运行（用于调试）
```bash
cd build/

# 终端 1
./uds_server

# 终端 2, 3, 4
./uds_client1
./uds_client2
./uds_client3
```

---

## 📊 项目统计

- **源代码文件**: 5 个（约 28KB）
- **测试脚本**: 3 个（约 16KB）
- **文档文件**: 4 个（约 18KB）
- **配置文件**: 3 个（约 12KB）
- **总计**: 15 个文件

---

## 🔍 文件依赖关系

```
┌─────────────────┐
│   config.hpp    │ ← 所有源文件的配置基础
└────────┬────────┘
         │
    ┌────┴────┬─────────┬─────────┐
    ↓         ↓         ↓         ↓
┌──────┐  ┌────────┐ ┌────────┐ ┌────────┐
│server│  │client1 │ │client2 │ │client3 │
│ .cpp │  │  .cpp  │ │  .cpp  │ │  .cpp  │
└──┬───┘  └───┬────┘ └───┬────┘ └───┬────┘
   └──────────┼──────────┼──────────┘
              ↓          ↓
       ┌──────────────────────┐
       │  CMakeLists.txt      │ ← 构建配置
       └──────────┬───────────┘
                  ↓
         ┌────────────────┐
         │   build.sh     │ ← 编译脚本
         └────────┬───────┘
                  ↓
         ┌────────────────┐
         │  run_test.sh   │ ← 测试脚本
         └────────┬───────┘
                  ↓
         ┌────────────────┐
         │    test.sh     │ ← 一键脚本
         └────────────────┘
```

---

## 🛠️ Makefile 命令速查

| 命令 | 功能 |
|------|------|
| `make` | 编译并测试 |
| `make build` | 只编译 |
| `make test` | 只测试 |
| `make debug` | Debug 模式 |
| `make clean` | 清理构建 |
| `make clean-all` | 完全清理 |
| `make rebuild` | 重新编译 |
| `make logs` | 查看日志 |
| `make info` | 项目信息 |
| `make help` | 帮助信息 |

---

## 💡 提示

- ⭐ **新手推荐**: 从 `QUICKSTART.md` 开始
- 📚 **深入学习**: 阅读 `README.md`
- 🔍 **查看成果**: 查看 `TEST_SUMMARY.md`
- 🚀 **快速测试**: 运行 `./test.sh` 或 `make`

---

**最后更新**: 2025-10-21  
**项目状态**: ✅ 完成并可用

