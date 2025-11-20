# ZeroCopy 2.0 架构骨架概览

本文基于现有 `zero_copy_framework` 代码树，总结出 ZeroCopy 2.0 版本的整体模块划分与主干逻辑，便于在后续版本演进中保持清晰的层次关系。整体自上而下分为 **应用层 → 通讯/控制层（Diroute）→ 内存/共享资源层 → 基础能力层（Posix/FoundationLib）**，并结合现有 `pusu_test` 测试工程描述典型用例。

---

## 1. 应用层（Daemon & 测试）

### 1.1 Daemon 入口：`zerocp_daemon/application`
- `diroute_main.cpp`：单进程入口，负责解析配置、安装信号处理、创建 `DirouteComponents` 共享内存、启动 `Diroute` 主循环与心跳线程，并在退出时清理 `MemPoolManager`。
- 该层面向运维/部署，暴露最小化 CLI，后续 2.0 可在此扩展运行参数（如动态配置、诊断指令）。

### 1.2 PUSU 测试矩阵：`test/zerocp_daemontest/pusu_test`
- `publisher_client.cpp` / `subscriber_client.cpp`：模拟运行时通过 Unix Domain Socket 连接到 Daemon，注册服务并发送/接收 chunk。
- `run_pusu_test.sh`：编排 daemon + 多客户端的端到端验证流程，是 2.0 保证兼容性与性能回归的首选脚本。
- `pusu_test_common.hpp`：封装客户端和 Daemon 之间的协议常量/辅助函数。
- 这些组件确保应用层场景可快速回归，也为 2.0 引入更多业务型用例提供模板。

---

## 2. 通讯/控制层（Diroute 核心）

### 2.1 共享组件容器：`zerocp_daemon/diroute`
- `diroute_components.hpp`：定义共享内存内的结构化组件（心跳池 `HeartbeatPool` + 最多 64 个 `LockFreeRingBuffer` 接收队列），采用分步构造和 offset 引用模型。
- 2.0 目标：保持兼容的二进制布局，必要时通过版本号/偏移表扩展字段，以支持更大队列或 QoS 标记。

### 2.2 路由调度：`zerocp_daemon/communication`
- `Diroute` 类（`communication/include/diroute.hpp`）承担注册/匹配/路由三大职责：
  - 管理 Publisher/Subscriber 的生命周期和心跳；
  - 为每个订阅者分配队列 offset，协调 `SharedChunk` 引用计数；
  - 通过后台线程扫描心跳池剔除失效进程。
- `popo/message_header.hpp`、`runtime` 子目录定义跨进程协议头及运行时辅助对象。
- 2.0 可以在此拓展：如多播策略、优先级队列、流控与监控指标。

### 2.3 进程通信协议
- 控制面使用 Unix Domain Socket（见 FoundationLib `posix`），数据面依赖共享内存队列。
- `service_description.hpp` 描述服务发现三元组（service/interface/event），是路由选择的关键输入。

---

## 3. 内存与共享资源层

### 3.1 内存管理：`zerocp_daemon/memory`
- `memory.md`：详述管理段(`/iox_mgmt`)与数据段(`/iox_publisher_group`)的分区布局，包括 Chunk 管理数组、不同尺寸的数据池（128B / 1KB / 4KB）。
- `include/heartbeat_pool.hpp`：提供固定容量、索引式的心跳槽位，供 Daemon 与客户端互相检测。
- 2.0 可在此补充：动态扩容策略、NUMA 亲和、跨节点同步等。

### 3.2 Chunk 生命周期：`zerocp_daemon/mempool`
- `shared_chunk.*`：为共享内存中的 chunk 提供引用计数和 RAII 包装，配合 `MemPoolManager` 避免重复构造/销毁。
- `mempool_allocator.*`：桥接 FoundationLib allocator，支撑零拷贝 chunk 分配。
- 未来 2.0 可以增加 chunk 追踪、压缩或加密扩展点。

---

## 4. 基础能力层（Foundation Library）

`zerocp_foundationLib` 提供跨模块可复用的底座，2.0 需继续保持清晰分层，建议对每个子目录给出职责边界：

| 模块 | 主要职能 | 关键文件示例 |
|------|----------|-------------|
| `posix/` | POSIX IPC、文件、socket 封装，含 Builder 模式与 error handling | `posix/memory/include/unix_domainsocket.hpp` |
| `report/` | 高性能日志组件（LockFreeRingBuffer、后台 worker、宏） | `report/README.md`, `report/source/*` |
| `memory/` | 通用内存工具（Bump allocator、固定池）驱动 mempool | `memory/include/bump_allocator.hpp` |
| `concurrent/` | 原子/锁封装及线程原语 | `concurrent/include/spinlock.hpp` (示例) |
| `design/` & `vocabulary/` | 轻量类型系统、Builder 宏、固定长度字符串等，确保共享内存可移植 | `design/include/builder_macros.hpp`, `vocabulary/include/fixed_string.hpp` |
| `core/` / `filesystem/` | 基础工具集、路径/文件接口 | `core/include/expected.hpp`, `filesystem/include/path.hpp` |

这些模块提供：错误传播(`std::expected`-style)、零拷贝日志、跨平台 IPC 封装，是构建 Daemon 和客户端的共同基石。

---

## 5. 2.0 版本建议的主干流程

1. **应用层引导**  
   - `diroute_main.cpp` 解析配置 → 初始化 `DirouteMemoryManager` → 构造 `DirouteComponents` → 启动控制层线程。
2. **控制层协作**  
   - `Diroute` 监听 Unix Domain Socket，接入 publisher/subscriber 请求，映射到 `DirouteComponents` 中的 receive queue 和心跳槽位。
3. **内存服务**  
   - `MemPoolManager` 从管理段装载 chunk 元数据；`SharedChunk` 将 handle 在 publisher/subscriber 之间传递并负责归还。
4. **基础库支撑**  
   - POSIX builder 管理 socket/shm 句柄；Report 日志记录各阶段事件；memory/concurrent/vocabulary 提供编译期安全保障。
5. **验证闭环**  
   - `pusu_test` 启动 daemon + publisher + subscriber，验证注册、心跳、消息路由全链路；可扩展为性能、容灾、QoS 等 2.0 场景。

---

## 6. 下一步演进（2.0 规划基线）

- **配置化与可观测性**：在应用层增加配置中心、日志级别/输出切换、REST 或 CLI 诊断入口。
- **Diroute 扩展**：支持多路由域、Topic 优先级、流控策略；引入指标上报。
- **内存升级**: 允许热扩容共享内存池、增加大块/自定义 size class；提供 chunk 生命周期追踪。
- **基础库强化**：统一错误码；POSIX 封装增加超时/重试；Report 日志集成 JSON/二进制落盘模式。
- **测试矩阵**：在 `pusu_test` 基础上加入 chaos/压力脚本，覆盖 2.0 新特性。

该文档可作为 2.0 架构蓝图，后续在对应目录下进一步细化设计或补充时序图/交互图即可。


