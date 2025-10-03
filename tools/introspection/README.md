# Introspection Tool - 运行时监控工具

## 概述

Introspection 工具是 Zero Copy Framework 的实时监控和可视化工具，通过 TUI 界面显示多进程系统的内存、进程、连接状态。它是多进程零拷贝框架的"眼睛"，让开发者能够实时了解系统运行状况。

## 功能特性

### 🔍 实时监控
- **内存监控**: 实时显示系统内存使用情况，包括总内存、已使用、空闲、共享、缓冲区、缓存等详细信息
- **进程监控**: 显示所有进程的详细信息，包括 PID、名称、内存使用、CPU 使用率、状态、命令行等
- **连接监控**: 监控网络连接状态，包括本地/远程地址、协议、状态、数据传输量等
- **系统负载**: 显示 CPU 使用率、负载平均值等系统负载信息

### 🎨 直观界面
- **TUI 界面**: 基于 ncurses 的文本用户界面，支持颜色显示和实时更新
- **多视图切换**: 支持概览、进程、连接、系统四种视图模式
- **实时刷新**: 可配置的更新间隔，默认 1 秒刷新一次
- **交互式操作**: 支持键盘快捷键操作，提供帮助信息

### 🔧 灵活配置
- **进程过滤**: 可以指定只监控特定名称的进程
- **连接过滤**: 可以指定只监控特定端口的连接
- **更新间隔**: 可以自定义数据更新频率
- **命令行参数**: 支持丰富的命令行选项

## 编译和安装

### 依赖要求
- CMake 3.16+
- C++17 编译器
- ncurses 库
- Linux 系统 (使用 /proc 文件系统)

### 编译步骤
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make introspection

# 安装 (可选)
sudo make install
```

### 快速编译
```bash
cd tools/introspection
mkdir build && cd build
cmake ..
make
```

## 使用方法

### 基本用法
```bash
# 启动默认监控
./introspection

# 显示帮助信息
./introspection --help
```

### 命令行选项
```bash
# 设置更新间隔为 500ms
./introspection --interval 500

# 只监控 nginx 和 apache 进程
./introspection --process nginx --process apache

# 只监控 80 和 443 端口
./introspection --connection 80 --connection 443

# 组合使用
./introspection -i 200 -p nginx -p redis -c 80 -c 6379
```

### TUI 快捷键
- `1-4`: 切换视图 (概览/进程/连接/系统)
- `h`: 显示/隐藏帮助信息
- `q`: 退出程序
- `r`: 刷新数据

## 界面说明

### 概览视图 (默认)
- 显示内存使用情况和系统负载
- 包含内存使用率进度条
- 显示详细的内存统计信息
- 显示 CPU 使用率和负载平均值

### 进程视图
- 显示前 20 个内存使用最多的进程
- 包含 PID、名称、内存使用、CPU 使用率、状态、命令行
- 按内存使用量排序
- 支持进程名称过滤

### 连接视图
- 显示网络连接信息
- 包含本地/远程地址、协议、状态、数据传输量
- 支持端口过滤
- 显示连接状态统计

### 系统视图
- 显示详细的系统信息
- 包含内存、CPU、负载等综合信息
- 提供系统性能概览

## 技术实现

### 架构设计
```
IntrospectionTool
├── IntrospectionMonitor (数据收集)
│   ├── 内存信息收集
│   ├── 进程信息收集
│   ├── 连接信息收集
│   └── 系统负载收集
└── IntrospectionTUI (界面显示)
    ├── 主界面绘制
    ├── 数据可视化
    ├── 用户交互
    └── 帮助系统
```

### 数据源
- **内存信息**: `/proc/meminfo`
- **进程信息**: `/proc/[pid]/status`, `/proc/[pid]/cmdline`, `/proc/[pid]/stat`
- **连接信息**: `/proc/net/tcp`, `/proc/net/udp`
- **系统负载**: `/proc/loadavg`

### 性能优化
- 多线程设计，数据收集和界面显示分离
- 可配置的更新间隔，平衡实时性和性能
- 智能过滤，减少不必要的数据处理
- 内存友好的数据结构设计

## 扩展功能

### 自定义监控
可以通过修改源代码来添加自定义监控功能：

1. **添加新的数据源**: 在 `IntrospectionMonitor` 中添加新的收集方法
2. **扩展数据结构**: 在 `SystemMetrics` 中添加新的字段
3. **自定义界面**: 在 `IntrospectionTUI` 中添加新的显示面板

### 集成到其他工具
Introspection 工具可以集成到其他监控系统中：

```cpp
#include "introspection.h"

// 创建监控器
auto monitor = std::make_shared<zero_copy::introspection::IntrospectionMonitor>();
monitor->start(1000); // 1秒更新间隔

// 获取监控数据
auto metrics = monitor->getCurrentMetrics();
// 处理数据...

// 停止监控
monitor->stop();
```

## 故障排除

### 常见问题
1. **编译错误**: 确保安装了 ncurses 开发包
2. **权限问题**: 某些系统信息需要适当权限
3. **显示问题**: 确保终端支持颜色和 UTF-8

### 调试模式
```bash
# 使用调试模式编译
cmake -DCMAKE_BUILD_TYPE=Debug ..
make introspection

# 运行调试版本
gdb ./introspection
```

## 贡献指南

欢迎贡献代码和功能改进！请遵循以下步骤：

1. Fork 项目
2. 创建功能分支
3. 提交更改
4. 创建 Pull Request

## 许可证

本项目采用 MIT 许可证。详见 LICENSE 文件。
