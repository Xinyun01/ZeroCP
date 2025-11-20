## ZeroCopy Middleware 1.x 完成度总结与 2.0 设计起点

> 基于 `zerocp_daemon` 与 `zerocp_foundationLib` 现有代码，整理已交付能力、目录结构、核心数据流及可直接继承的基础设施，作为 ZeroCopy 2.0 版本的起点文档。

---

### 1. 代码树全景

```
zero_copy_framework
├── zerocp_daemon
│   ├── application          # Daemon 入口 (diroute_main.cpp)
│   ├── communication        # Diroute 控制层，含 popo/runtime 协议
│   ├── diroute              # 共享组件容器 DirouteComponents
│   ├── memory               # 心跳池、shm 布局、内存管理文档
│   └── mempool              # Chunk/管理池/allocator RAII
├── zerocp_foundationLib
│   ├── posix                # Unix Domain Socket & SHM builder
│   ├── report               # 无锁异步日志系统 (LockFreeRingBuffer)
│   ├── memory               # 通用 allocator & pool primitives
│   ├── concurrent           # Spinlock / atomic 工具
│   ├── core                 # expected、error handling、utility
│   ├── filesystem           # path/file 抽象
│   ├── design               # builder 宏、类型萃取
│   └── vocabulary           # fixed_string 等轻量类型
└── test/zerocp_daemontest/pusu_test
    ├── publisher_client.cpp
    ├── subscriber_client.cpp
    ├── pusu_test_common.hpp
    └── run_pusu_test.sh
```

---

### 2. 应用层 (Daemon 与测试矩阵)

- `zerocp_daemon/application/diroute_main.cpp`：解析 CLI → 初始化 `DirouteComponents` 和 `MemPoolManager` → 启动 `Diroute` 主循环及心跳线程 → 负责退出清理，已经具备生产运维入口雏形。
- `test/zerocp_daemontest/pusu_test`：提供 daemon + publisher/subscriber 端到端脚本，验证注册、共享内存 chunk 流转与心跳剔除，是 2.0 的功能/性能回归基线。
- 已完成的信号处理、配置解析和测试脚本可直接复用，2.0 可在此扩展诊断 CLI、动态配置、运行期指标导出。

---

### 3. Diroute 控制层

- `zerocp_daemon/diroute/diroute_components.hpp`：共享内存容器，内置 `HeartbeatPool` 与 64 个 `LockFreeRingBuffer<MessageHeader,1024>` 接收队列，通过 placement-new + base offset 方式实现在共享段中的自描述布局。
- `zerocp_daemon/communication/include/diroute.hpp` (及 `source/`)：管理 Publisher/Subscriber 生命周期、心跳租约、服务描述三元组匹配以及队列分配；背靠 `DirouteComponents` 暴露的 offset 接口完成跨进程引用。
- `communication/include/popo/message_header.hpp`、`runtime/*`：定义 chunk 元数据头、运行时上下文对象，已经支持零拷贝句柄在注册方之间传递。
- 现有代码提供基础注册路由与心跳剔除逻辑，2.0 可以新增：多队列优先级、流控/限速、监控指标挂钩、版本化的共享内存布局。

---

### 4. 内存与 Chunk 管理

- `zerocp_daemon/memory/memory.md`：详细描述 `/iox_mgmt` 管理段和 `/iox_publisher_group` 数据段的布局，包括 MemPool 索引数组、ChunkManagement 对象、不同 size class 的 chunk 实例，奠定了 170 个 chunk（128B/1KB/4KB）预配置池。
- `zerocp_daemon/memory/include/heartbeat_pool.hpp`：提供固定槽位的心跳池，实现索引式注册/释放与跨进程探活。
- `zerocp_daemon/mempool/`：
  - `shared_chunk.*`：封装 chunk 引用计数、生命周期钩子和 RAII 语义，确保 chunk 在 publisher/subscriber 之间安全复用。
  - `mempool_allocator.*`：关联 FoundationLib allocator，将共享内存块映射为可分配资源。
- 这些模块让共享内存读取/写入路径无锁运行，2.0 可关注：动态扩容、NUMA 亲和、chunk trace/监控、压缩或加密插件化。

---

### 5. Foundation Library 能力盘点

- `posix/`：封装 Unix Domain Socket、SHM、文件描述符管理，采用 Builder + expected 错误返回，为 Daemon 与客户端通讯提供统一抽象。
- `report/`：高性能异步日志系统（README 提供详解），具备 LockFreeRingBuffer、后台 worker、七级别日志、cache line 对齐与固定缓冲区设计。
- `memory/`：Bump allocator、固定 mempool、RelativePointer 支撑共享内存对象模型。
- `concurrent/`：Spinlock、原子封装、线程原语，是 Diroute 背景线程与锁自由结构的基础。
- `design/` & `vocabulary/`：Builder 宏、固定长度字符串、类型安全别名，保障共享内存结构在 ABI 上一致。
- `core/`、`filesystem/`：expected/error、路径/文件工具，支撑 CLI、配置与日志落盘。
- FoundationLib 已可满足 1.x 全链路需求，2.0 侧重统一错误码、增强 POSIX 超时/重试策略、扩展日志输出（JSON/二进制）、补充跨平台抽象。

---

### 6. 运行与验证路径

1. **启动**：`diroute_main.cpp` 解析参数 → 初始化共享内存 → 启动 `Diroute` 控制线程与心跳维护。
2. **注册/路由**：客户端通过 Unix Domain Socket（FoundationLib POSIX）发送服务描述；`Diroute` 为其分配 `HeartbeatPool` 槽位和 `LockFreeRingBuffer` offset。
3. **共享内存数据面**：Publisher 从 `MemPoolManager` 申请 chunk → 填充 payload → 将 handle 推入订阅者队列；Subscriber 取出 offset → 通过 `SharedChunk` 解引用并消费。
4. **回收**：`SharedChunk` 引用计数归零触发 chunk 归还；心跳线程剔除失效客户端并回收资源。
5. **验证脚本**：`test/zerocp_daemontest/pusu_test/run_pusu_test.sh` 自动拉起 daemon + 多客户端，覆盖注册、心跳、数据传输，适合作为 CI 场景。

---

### 7. 面向 ZeroCopy 2.0 的落地建议

- **应用层增强**：在现有 CLI 基础上增加配置热加载、诊断命令、健康检查 REST/CLI；将 `pusu_test` 扩展为压力/混沌脚本，形成回归矩阵。
- **Diroute 进化**：引入多域路由、优先级/流控策略、指标采集（队列长度、心跳延迟）、共享内存布局版本号以兼容升级。
- **内存服务升级**：支持运行期扩容/收缩、NUMA/多节点亲和、chunk trace（谁持有/阻塞）、可选 payload 加密/压缩插件。
- **FoundationLib 强化**：统一错误码体系、POSIX builder 增加超时/自动重试、Report 日志支持多后端（文件/Socket/JSON）、memory 模块提供更多 size class 配置工具。
- **工程化**：补充 CMake 目标/打包脚本、容器化部署样例、覆盖率与静态分析 pipeline，将 1.x 资产平滑迁入 2.0。

---

### 8. 后续行动 CheckList

- [ ] 以本文档为蓝本，细化 `Diroute` 2.0 功能需求与共享内存 ABI 变更策略
- [ ] 补充 FoundationLib 各子模块 README，统一 API 规范与示例
- [ ] 扩展 `pusu_test` 用例，纳入性能/容灾/长连稳定性场景
- [ ] 规划配置/可观测性组件（配置中心、日志、指标）的落地里程碑
- [ ] 梳理 2.0 关键技术债（动态扩容、QoS、错误码体系）并确立优先级

---

### 9. 子模块主要接口速查

#### zerocp_daemon/application
- `diroute_main.cpp`：`signalHandler(int)` 管控 SIGINT/SIGTERM/SIGUSR1，`main(int,char**)` 依次调用 `DirouteMemoryManager::createMemoryPool()`、`MemPoolManager::createSharedInstance()`、`Diroute::run()/stop()`，并通过 `Diroute::printRegisteredProcesses()` 响应手动诊断。

#### zerocp_daemon/diroute
- `diroute_components.hpp`：`constructHeartbeatPool()`、`heartbeatPool()`、`constructReceiveQueues()`、`acquireQueue()/releaseQueue()`、`getQueueOffset()/getQueueByOffset()` 将 HeartbeatPool 与 `LockFreeRingBuffer` 队列布局在共享段内。
- `diroute_memory_manager.hpp`：提供 `DirouteMemoryManager::createMemoryPool(const Config&)`、`getComponents()`、`getHeartbeatPool()`、`isInitialized()`，封装 POSIX SHM 创建、placement-new 构造和多阶段初始化。

#### zerocp_daemon/communication
- `include/diroute.hpp`：`Diroute::run()/stop()`、`startProcessRuntimeMessagesThread()`、`registerProcess()`、`handlePublisherRegistration()`、`handleSubscriberRegistration()`、`handleMessageRouting()`、`matchSubscribers()`、`routeMessageToSubscriber()`、`printRegisteredProcesses()` 构成控制/路由主流程，并带有心跳监控线程。
- `include/service_description.hpp`：`ServiceDescription::getService()/getInstance()/getEvent()` 标准化 service-instance-event 三元组，是所有注册/匹配接口的输入。
- `include/popo/publisher.hpp`：`Publisher::offer()/stopOffer()`、`loan()`、`LoanedSample::publish()`、`routeChunk()` 组合 MemPoolManager + Runtime 完成零拷贝发送。
- `include/popo/subscriber.hpp`：`Subscriber::subscribe()/unsubscribe()`、`take()`、`mapReceiveQueue()`、`Sample::header()` 完成接收侧注册、队列映射与 chunk 读取。
- `include/runtime/*`：`ProcessManager::registerProcess()/unregisterProcess()/isProcessRegistered()`、`IpcInterfaceCreator::createRuntimeInterface()`、`MessageRuntime::requestReply()`、`PoshRuntime::getInstance()/getHeartbeatSlotIndex()/getSharedMemoryBaseAddress()` 负责客户端 runtime 生命周期与 IPC。

#### zerocp_daemon/memory
- `mempool_manager.hpp`：`MemPoolManager::createSharedInstance()`、`attachToSharedInstance()`、`getInstanceIfInitialized()`、`getChunk()`、`releaseChunk()`、`getChunkManagerByIndex()`、`getManagementMemorySize()`、`destroySharedInstance()` 构成共享内存池的核心接口。
- `heartbeat_pool.hpp`：`HeartbeatPool::emplace()/release()`、`iteratorFromIndex()`、`for_each()`、`size()/capacity()` 支撑心跳租约管理。
- `chunk_manager.hpp`、`chunk_header.hpp`：暴露 `ChunkManager::incrementReaderCount()`、`decrementReaderCount()`、`getUserPayload()`、`getChunkIndex()` 等数据面接口。
- `mempool_config.hpp` / `mempool.hpp`：`MemPoolConfig::setDefaultPool()`、`MemPool::getPoolId()/acquireChunk()/releaseChunk()` 用来定义 size class。
- `posixshm_provider.hpp`：`PosixShmProvider::create()`、`map()`、`destroy()` 与 `MemPoolIntrospection` 配合完成 `/iox_mgmt` 与 `/iox_publisher_group` 的映射。

#### zerocp_daemon/mempool
- `shared_chunk.hpp`：`SharedChunk::SharedChunk()`、`reset()`、`useCount()`、`getUserPayload()`、`getChunkManagerIndex()`、`prepareForTransfer()`、`SharedChunk::fromIndex()` 为 chunk 句柄提供 RAII 及跨进程重建能力。
- `chunksetting.hpp`：集中定义 `ChunkSetting::chunkCount/sizeClass` 等常量，供 `MemPoolConfig` 计算共享内存大小。
- `CROSS_PROCESS_TRANSFER.md` / `SHARED_CHUNK_USAGE.md`：配套文档解释 `prepareForTransfer()` + `fromIndex()` 协议及引用计数约束。

#### zerocp_foundationLib/posix
- `memory/include/posix_sharedmemory_object.hpp`：`PosixSharedMemoryObject::create()/open()`、`map(address,size)`、`getMappedAddress()`、`unlink()` 构建 Diroute & mempool 所需的共享段。
- `memory/include/posix_sharedmemory.hpp` 与 `posix_memorymap.hpp`：封装 `PosixSharedMemory::createAnonymous()/createNamed()` 与 `PosixMemoryMap::mapFixed()/unmap()`，提供多段映射。
- `memory/include/relative_pointer.hpp`：`RelativePointer::fromAbsolute()`、`toAbsolute()`、`baseAddress().set()`，保证跨进程指针稳定。
- `memory/include/unix_domainsocket.hpp`：`UnixDomainSocketBuilder::name()/channelSide()/create()`、`UnixDomainSocket::sendTo()/receiveFrom()/setReceiveTimeout()`、`destroy()` 为控制面 IPC 提供 builder + expected 语义。

#### zerocp_foundationLib/report
- `include/logging.hpp`：宏 `ZEROCP_LOG(level, msg)`、`ZEROCP_LOG_SET_LEVEL()`、`ZEROCP_LOG_INIT()` 管理日志入口。
- `include/lockfree_ringbuffer.hpp`：`LockFreeRingBuffer::push()/pop()/size()`、`reserve()` 被 Diroute 队列与日志 backend 复用。
- `include/log_backend.hpp` / `logstream.hpp`：`LogBackend::submit()`、`LogStream::operator<<` 支持异步写入与格式化。

#### zerocp_foundationLib/memory
- `include/bump_allocator.hpp`：`BumpAllocator::allocate()/reset()/remaining()` 提供常量时间分配。
- `include/memory.hpp`：`alignUp()/alignDown()`、`constexprMemCpy()` 等编译期工具为共享内存布局服务。

#### zerocp_foundationLib/concurrent
- `include/mpmclockfreelist.hpp`：`MpmcLockFreeList::push()/pop()/size()` 模板在日志、chunk 追踪等场景提供无锁容器。

#### zerocp_foundationLib/design
- `builder.hpp`：`ZeroCP_Builder_Implementation` 宏生成链式 builder，配合 POSIX/日志模块；`linux_name.hpp` 定义 `LinuxName::fromPath()` 等辅助。

#### zerocp_foundationLib/filesystem
- `include/filesystem.hpp` / `filesystemInterface.hpp`：`Filesystem::exists()/remove()/createDirectories()`、`FilesystemInterface::open()/read()/write()` 作为 Unix 句柄抽象。

#### zerocp_foundationLib/vocabulary
- `include/string.hpp`、`fixed_position_container.hpp`、`vector.hpp`：`ZeroCP::string<N>`、`FixedPositionContainer::emplace()/iter_from_index()`、`vector::push_back()/erase()` 提供 ABI 稳定的数据结构；`algorithm.hpp` 集中存放 constexpr 工具。

---

### 10. 代码规模与复杂度刻度

- **总体行数**：`zerocp_daemon` ≈ 7.6k 行、`zerocp_foundationLib` ≈ 5.4k 行，核心逻辑集中在 Daemon 控制链路。
- **Daemon 子模块占比**：
  - `communication` ≈ 3.2k 行，覆盖 `Diroute`, `Publisher/Subscriber`, runtime IPC。
  - `memory` ≈ 2.8k 行，定义 mempool 布局、`HeartbeatPool`、`ChunkManager`。
  - `mempool` ≈ 1.1k 行，封装 `SharedChunk`/allocator。
  - `diroute` ≈ 0.5k 行，主要是共享组件容器。
  - `application` ≈ 0.15k 行，聚焦 CLI/入口。
- **FoundationLib 子模块占比**：
  - `posix` ≈ 2.1k 行：共享内存/Uds builder。
  - `report` ≈ 1.8k 行：异步日志管线。
  - `vocabulary` ≈ 0.8k 行：固定容量容器。
  - 其余（`design`、`memory`、`filesystem`、`concurrent`）共约 0.7k 行，提供编译期/并发/FS 支撑。
- 这些数据可作为 2.0 迭代时的工作量参考：若扩展 Diroute 控制层或 POSIX builder，需要优先投入在上述高占比模块。

### 11. 组件协作深描与扩展点

#### Diroute 控制面
- **入口串联**：`diroute_main.cpp` 在 `signalHandler` 中托管退出事件，`main()` 依次完成 `DirouteMemoryManager::createMemoryPool()` → `MemPoolManager::createSharedInstance()` → `Diroute::run()`，确保共享段先行准备，控制线程随后启动。
- **共享组件注入**：`diroute_components.hpp` 把 `HeartbeatPool`、`LockFreeRingBuffer`、`ReceiveQueueGroup` placement-new 在 `/iox_mgmt`，通过 `RelativePointer` 实现 offset 序列化，供 `Diroute::matchSubscribers()` 直取。
- **控制协议**：`communication/include/diroute.hpp` 调度 `registerProcess()`/`handlePublisherRegistration()`/`handleMessageRouting()`，内部依赖 `runtime/posh_runtime.hpp` 提供 Unix Domain Socket 回复能力，并由 `startProcessRuntimeMessagesThread()` 常驻处理客户端心跳/控制消息。
- **扩展建议**：在 `Diroute::run()` 的 polling loop 中加入优先级调度/指标采样，可复用 `report/log_backend.hpp` 写入；心跳与流控参数可通过 `diroute_main` CLI 暴露，以便 2.0 做热加载。

#### 内存与 Chunk 服务
- **Mempool 构建**：`MemPoolManager::createSharedInstance()` 结合 `MemPoolConfig::setDefaultPool()` 与 `ChunkSetting`，在 `/iox_publisher_group` 中预配 128B/1KB/4KB 等 size class；`getManagementMemorySize()` 提供部署期容量估算。
- **Chunk 生命周期**：`SharedChunk::prepareForTransfer()` 将 `ChunkHeader` 的管理索引编码进句柄，`SharedChunk::fromIndex()` 在 Subscriber 侧反解，同时 `ChunkManager::incrementReaderCount()`/`decrementReaderCount()` 保证引用计数安全。
- **心跳池**：`HeartbeatPool::emplace()`/`release()` 让每个 runtime 占用一个槽位，`for_each()` 可在监控线程中遍历，结合 `PoshRuntime::getHeartbeatSlotIndex()` 实现跨进程租约。
- **扩展建议**：2.0 可在 `ChunkManager` 层记录 `lastAccessTimestamp`，与 `report` 模块联动生成慢消费者告警；也可以在 `MemPoolConfig` 引入 NUMA-aware size class 映射。

#### FoundationLib 关键能力
- **POSIX Builder**：`posix_sharedmemory_object.hpp` + `posix_sharedmemory.hpp` 提供 `create()/open()`、`mapFixed()`，与 `RelativePointer` 组合解决多进程地址稳定性；`unix_domainsocket.hpp` 用链式 builder 收敛 fd 参数，易于在 Daemon 与客户端保持一致。
- **Report 日志**：`logging.hpp` 宏入口 + `LockFreeRingBuffer` + `log_backend.hpp` 构成单生产者多消费者的日志通道，`LogBackend::submit()` 支持把 Diroute 指标/异常直接输出到文件或 socket。
- **Vocabulary & Design**：`ZeroCP_Builder_Implementation` 简化 builder 模式生成，`ZeroCP::string<N>` 保障共享内存 ABI 固定；`FixedPositionContainer`/`vector` 提供索引式容器便于 `HeartbeatPool` 等结构避免动态分配。
- **扩展建议**：为 2.0 引入统一错误码，可在 `core` 模块增补 `expected` 包装器，并在 `posix`/`report`/`communication` 中透传；同时 `filesystem` 可配合 CLI 提供配置热加载路径探测。

#### 运行/测试闭环
- `pusu_test` 以 Bash 驱动 publisher/subscriber 多进程，脚本已覆盖注册、心跳、数据流三条主路径。可在 2.0 中扩展为压力/混沌模式：例如借助 `run_pusu_test.sh` 增加负载参数，记录 `LockFreeRingBuffer::size()` 指标。
- 建议在 CI 中组合 `diroute_main --dry-run`（验证共享段创建）+ `pusu_test`（功能）+ FoundationLib 单元测试（`posix/tests` 若补齐），形成“入口→控制→数据→基础库”多层守护。

---

### 12. 逐文件接口职责清单

#### zerocp_core/source
- `ipc_channel.cpp`：当前仅保留骨架注释，预留给进程间通道抽象的实现；与 `include/ipc_channel.hpp` 对应，后续可补充 send/receive/close 等接口。
- `memory_pool.cpp`：空壳文件，计划承载 `MemoryPool` 的页分配与统计逻辑。
- `shared_memory.cpp`：空壳文件，预留共享内存 RAII 封装，未来与 FoundationLib POSIX 构件对接。

#### zerocp_daemon/application
- `diroute_main.cpp`：`signalHandler()` 捕获 SIGINT/SIGTERM/SIGUSR1，`main()` 串联 `DirouteMemoryManager::createMemoryPool()` → `MemPoolManager::createSharedInstance()` → `Diroute::run()/stop()`，并维护守护进程的 Heartbeat 槽位。

#### zerocp_daemon/diroute
- `diroute_memory_manager.cpp`：`DirouteMemoryManager::createMemoryPool()` 通过 `PosixSharedMemoryObjectBuilder` 映射 `/iox_mgmt`，随后在共享段内 placement-new 出 `DirouteComponents`、`HeartbeatPool`、`ReceiveQueueGroup` 并可用 `getHeartbeatPool()/getComponents()` 对外暴露。

#### zerocp_daemon/communication
- `source/diroute.cpp`：`Diroute::run()/stop()` 驱动心跳与 IPC 线程，`processRuntimeMessagesThread()` 解析 `REGISTER/PUBLISHER/SUBSCRIBER/ROUTE` 协议并回写响应，`handleProcessRegistration()`/`handlePublisherRegistration()` 等接口与共享内存实体交互，`checkHeartbeatTimeouts()` 在 3 秒阈值内淘汰失联 runtime。
- `source/popo/posh_runtime.cpp`：实现客户端 Runtime 单例，`initRuntime()/getInstance()` 负责自举，`initializeConnection()/registerToRouteD()` 使用 UDS request-reply 拿到 `OK:OFFSET:<slot>`，随后 `openHeartbeatSharedMemory()/registerHeartbeatSlot()/startHeartbeat()` 将心跳写入 `/iox_mgmt` 并提供 `requestReply()/sendMessage()`。
- `source/runtime/ipc_interface_creator.cpp`：`IpcInterfaceCreator::createUnixDomainSocket()` 用 builder 快速构造 server/client UDS，`sendMessage()/receiveMessage()` 管理 server→client 回复地址。
- `source/runtime/ipc_runtime_interface.cpp`：封装进程本地 runtime 通信的占位实现，目前仅构造 `MessageRuntime`，其余方法待补。
- `source/runtime/message_runtime.cpp`：`MessageRuntime` 构造时快照 `pid/uid/timestamp/appName` 并提供 `get_pid()/get_timestampNs()` 等 accessor，供控制消息打标签。
- `source/runtime/process_manager.cpp`：`ProcessManager::initRuntime()/getInstance()` 保证单例；`registerProcess()/unregisterProcess()/getProcessInfo()` 维护注册表；`sendMessageToProcess()` 通过 `IpcRuntimeInterface` 推送控制消息；`getMonitoredProcesses()`、`dumpProcessInfo()` 便于 CLI/日志输出。

#### zerocp_daemon/memory
- `chunk_manager.cpp`：暂未落地，实现入口用于 `ChunkManager` 具体逻辑。
- `mempool_allocator.cpp`：负责共享内存布局，`ManagementMemoryLayout()` 把每个 `MemPool` 的 free list、`ChunkManagerPool` placement-new 到 `/iox_mgmt`，`ChunkMemoryLayout()` 依据 `MemPoolConfig` 将 `[ChunkHeader+payload]` 的数据区排布在 `/iox_publisher_group`。
- `mempool_config.cpp`：`MemPoolConfig::addMemPoolEntry()/setdefaultPool()` 收敛 size-class 配置，并提供 copy ctor/assign 保障在共享内存中的安全复制。
- `mempool_manager.cpp`：核心管理器，`createSharedInstance()/attachToSharedInstance()` 负责创建/挂载管理&数据段，借助命名信号量确保单次初始化；对外暴露 `getChunk()/releaseChunk()/getChunkManagerByIndex()` 等接口，并提供 `destroySharedInstance()` 做共享资源回收。
- `mempool.cpp`：`MemPool` 构造时把 `RelativePointer`、`MPMC_LockFree_List` 与 chunk 元信息绑定，`setRawMemory()` 用 `dataOffset` 让多进程能通过偏移重建物理地址。
- `posixshm_provider.cpp`：`PosixShmProvider::createMemory()` 使用 FoundationLib builder 统一创建/映射共享段并登记 poolId，`destroyMemory()` 负责 unmap + 反注册。

#### zerocp_daemon/mempool
- `shared_chunk.cpp`：提供 chunk 跨进程 RAII，`SharedChunk::prepareForTransfer()` bump 引用计数并返回句柄索引，`SharedChunk::fromIndex()` 在订阅侧借 `MemPoolManager::getChunkManagerByIndex()` 复原指针，`release()` 在析构/重置时调用 `MemPoolManager::releaseChunk()`。

#### zerocp_foundationLib/posix/memory
- `posix_memorymap.cpp`：`PosixMemoryMapBuilder::create()` wrap `mmap/munmap` 并将 `AccessMode` 转换为 `PROT_*`，`PosixMemoryMap` 提供 `getBaseAddress()/getLength()` 及 move 语义。
- `posix_sharedmemory_object.cpp`：`PosixSharedMemoryObjectBuilder::create()` 组合 `PosixSharedMemoryBuilder` + `PosixMemoryMapBuilder`，校验 size，再交付 `PosixSharedMemoryObject`，其 `getBaseAddress()/getFileHandle()` 供上层复用。
- `posix_sharedmemory.cpp`：`PosixSharedMemoryBuilder::create()` 负责 `shm_open/ftruncate` 生命周期及权限校验，`PosixSharedMemory` 在析构时依据所有权 unlink 对应 shm。
- `unix_domainsocket.cpp`：`UnixDomainSocketBuilder::create()` 检查名称长度、创建/绑定 `AF_UNIX`，`UnixDomainSocket::sendTo()/receiveFrom()/setReceiveTimeout()` 提供基于 datagram 的 IPC 能力，并在服务端关闭时 unlink socket 文件。

#### zerocp_foundationLib/report/source
- `logging.cpp`：`Log_Manager` 构造时创建 `LogBackend` 并启动线程，`start()/stop()` 控制后台消费。
- `log_backend.cpp`：`LogBackend::start()/stop()` 与 `workerThread()` 组成 SPSC 队列消费者，`submitLog()` 支持带丢弃计数的入队。
- `logstream.cpp`：`LogStream` 收集 `operator<<` 输入，在析构中拼接 `[timestamp][level][file:line]` 前缀并把缓冲推入 `Log_Manager` 后端。
- `lockfree_ringbuffer.cpp`：实现 `LogMessage` 拷贝/赋值/序列化逻辑，供模板环形缓冲序列化日志。

#### zerocp_foundationLib/memory/source
- `bump_allocator.cpp`：`BumpAllocator::allocate()` 基于 `align()` 计算偏移并返回 monotonic 地址，错误时通过 `std::expected` 传递。
- `memory.cpp`：提供 `align()`、`alignedAlloc()/alignedFree()`，供共享内存布局和对齐工具复用。

#### zerocp_foundationLib/concurrent/source
- `mpmclockfreelist.cpp`：无锁空闲链表实现，`Initialize()` 排列索引，`pop()/push()` 借 ABA counter 与 `std::atomic` 保证多生产者多消费者安全。

#### zerocp_foundationLib/filesystem/source
- `filesystemInterface.cpp`：尚未实现，预留面向 `FilesystemInterface` 的具体 POSIX 读写封装。

---

> 现有 1.x 代码已覆盖入口、控制层、共享内存和基础库四个面，具备构建 ZeroCopy 2.0 的可复用骨架。后续只需围绕扩展性、可观测性和工程化完善即可。

---

### 13. zerocp_daemon 全量文件骨架与接口

```
zerocp_daemon
├── application
├── communication
│   ├── include
│   │   ├── diroute.hpp
│   │   ├── popo/
│   │   ├── routemsg/
│   │   ├── runtime/
│   │   └── service_description.hpp
│   └── source
│       ├── diroute.cpp
│       ├── popo/
│       └── runtime/
├── diroute
├── mempool
├── memory
│   ├── include
│   └── source
└── ...
```

#### application
- `application/diroute_main.cpp`：守护进程入口。安装 `signalHandler`，初始化 `DirouteMemoryManager` 与 `MemPoolManager`，再驱动 `Diroute::run()/stop()` 并保持心跳。

#### communication/include
- `diroute.hpp`：声明 Diroute 控制平面的对外接口（注册、路由、心跳检查、IPC 线程管理等），作为服务端主调入口。
- `service_description.hpp`：描述发布/订阅服务 ID、instance/event，供注册与匹配使用。
- `popo/message_header.hpp`：定义路由层消息头部字段（chunk index、payload size、timestamp 等）。
- `popo/posh_runtime.hpp`：声明 `PoshRuntime` 单例接口（`initRuntime/getInstance/sendMessage/requestReply` 等）和 SHM/心跳交互方法。
- `popo/publisher.hpp`：发布端抽象骨架，提供 `loan()`/`publish()`/`offer()` 等接口声明，尚未完全实现。
- `popo/subscriber.hpp`：订阅端接口骨架，声明 `subscribe()`/`take()`/`release()` 等方法。
- `routemsg/routemsg.hpp`：封装 `REGISTER/PUBLISHER/SUBSCRIBER/ROUTE` 协议字段与解析工具。
- `runtime/heatbeat.hpp`：定义 runtime 心跳 payload/slot 结构。
- `runtime/ipc_interface_creator.hpp`：声明 Unix Domain Socket 构建器（client/server）以及 send/receive API。
- `runtime/ipc_runtime_interface.hpp`：抽象 runtime 进程间控制通道的接口（连接、发送、关闭等）。
- `runtime/message_runtime.hpp`：定义 `MessageRuntime` 元数据容器（pid/uid/appName/timestamp）。
- `runtime/processinfo.hpp`：描述受管进程状态（pid、名称、上次心跳、角色等）。
- `runtime/process_manager.hpp`：声明进程注册、心跳监控、消息转发等管理接口。

#### communication/source
- `source/diroute.cpp`：控制面实现，提供注册/注销/心跳检查/路由匹配线程，并驱动 `ProcessManager` 与 mempool。
- `source/popo/posh_runtime.cpp`：PoshRuntime 具体实现，负责 UDS 建立、RouteD 注册、共享内存心跳槽获取与心跳线程。
- `source/runtime/ipc_interface_creator.cpp`：Unix Domain Socket 封装实现，完成 `createUnixDomainSocket()` 与基础 send/recv。
- `source/runtime/ipc_runtime_interface.cpp`：Runtime IPC 层占位实现，当前仅连接 `MessageRuntime`，为 2.0 预留拓展。
- `source/runtime/message_runtime.cpp`：实现 `MessageRuntime` 构造与 accessor。
- `source/runtime/process_manager.cpp`：实现受管进程注册表、心跳超时检查、向具体 runtime 发送控制消息等逻辑。

#### diroute
- `diroute_components.hpp`：定义共享内存中的全部组件（HeartbeatPool、ReceiveQueue、MemPool 接口）的聚合体。
- `diroute_memory_manager.hpp/.cpp`：封装 `/iox_mgmt` 的映射、组件 placement-new、HeartbeatPool/DirouteComponents 的访问接口。

#### mempool
- `CROSS_PROCESS_TRANSFER.md`：描述 chunk 句柄在跨进程转移时的约束和序列化规则。
- `SHARED_CHUNK_USAGE.md`：说明 `SharedChunk` 的生命周期、引用计数和使用示例。
- `chunksetting.hpp`：定义 size class/数量等静态配置结构。
- `shared_chunk.hpp/.cpp`：Chunk RAII 封装，实现 prepare/attach/release，以及 mempool 句柄互转。

#### memory/include
- `chunk_header.hpp`：定义 chunk header 元信息（大小、refcount、payload offset 等）。
- `chunk_manager.hpp`：声明 chunk 管理器接口（loan、release、reader count 操作）。
- `default_memorypool.hpp`：提供默认 mempool 组合配置。
- `heartbeat.hpp`：定义 runtime heartbeat 数据结构及状态标记。
- `heartbeat_pool.hpp`：声明起租、释放、遍历心跳槽的方法。
- `mempool_allocator.hpp`：声明管理区/数据区布局构建器。
- `mempool_config.hpp`：配置入口，提供 add entry/default set 等接口。
- `mempool.hpp`：声明 mempool 本体、持有 chunk header/raw memory 指针。
- `mempool_introspection.hpp`：暴露调试/监控接口（统计、dump 等）。
- `mempool_manager.hpp`：管理共享实例的接口（create/attach/getChunk/destroy）。
- `posixshm_provider.hpp`：声明 POSIX 共享内存提供者（create/destroy/open attach）。

#### memory/source
- `chunk_manager.cpp`：待实现骨架，预留 chunk 生命周期管理的具体逻辑。
- `mempool_allocator.cpp`：实现共享内存布局算法，分配 management & chunk 区域。
- `mempool_config.cpp`：实现配置项增删、默认集生成、复制赋值。
- `mempool.cpp`：实现 mempool 初始化与 `setRawMemory()`。
- `mempool_manager.cpp`：实现共享实例创建/挂载、chunk 发放、引用计数更新等。
- `posixshm_provider.cpp`：实现 POSIX 共享段 create/destroy/open。
- `memory.md`：记录整体内存设计要点。
- `shared_memory_layout_detailed.md`：详细描述共享段布局、offset 表和同步策略。

### 14. zerocp_foundationLib 全量文件骨架与接口

```
zerocp_foundationLib
├── concurrent
├── core
├── design
├── filesystem
├── memory
├── posix
├── report
└── vocabulary
```

#### concurrent
- `README.md`：概述 MPMC lock-free freelist 设计与使用方法。
- `include/mpmclockfreelist.hpp`：声明无锁 freelist 模板接口（push/pop/init）。
- `source/mpmclockfreelist.cpp`：实现细节，处理 ABA 计数与 slot 初始化。

#### core
- `include/`：当前为空骨架，预留通用核心接口（例如 expected/utility）。

#### design
- `builder.hpp`：提供 Builder 模式模板，支持链式配置 `TBuilder`.
- `linux_name.hpp`：封装 POSIX 名称规范（shm/uds）与校验工具。

#### filesystem
- `include/filesystem.hpp`：声明跨平台文件系统辅助函数集合。
- `include/filesystemInterface.hpp`：定义可替换的文件系统接口（open/read/write 等）。
- `source/filesystemInterface.cpp`：占位实现，等待具体 POSIX 封装。

#### memory
- `include/bump_allocator.hpp`：声明 `BumpAllocator` 与 `BumpAllocatorView`，面向共享内存一次性分配。
- `include/memory.hpp`：对齐、分配工具函数声明。
- `source/bump_allocator.cpp`：实现连续分配/重置及错误处理。
- `source/memory.cpp`：实现 `align()`、`alignedAlloc()` 等基础工具。

#### posix/memory
- `LOGGING_GUIDE.md`：记录 POSIX 组件的日志规范。
- `RELATIVE_POINTER_USAGE.md`：指导如何安全使用 relative pointer。
- `deital/relative_pointer.inl`：提供 relative pointer inline 实现。
- `include/posix_memorymap.hpp`：声明内存映射 builder/对象接口。
- `include/posix_sharedmemory.hpp`：声明共享内存创建/打开接口。
- `include/posix_sharedmemory_object.hpp`：声明组合对象（shm+map）。
- `include/relative_pointer.hpp`：声明 offset-based 指针工具。
- `include/unix_domainsocket.hpp`：声明 Unix Domain Socket builder/对象。
- `source/posix_memorymap.cpp`：实现 mmap 封装。
- `source/posix_sharedmemory.cpp`：实现 shm_open/ftruncate/close。
- `source/posix_sharedmemory_object.cpp`：组合 builder，暴露句柄与地址。
- `source/unix_domainsocket.cpp`：实现 UDS builder、send/receive 等。

#### posix/posixcall/include
- `posix_call.hpp`：声明对 `read/write/open/...` 的包装接口。
- `detial/posix_call.inl`：提供 `errno` 处理的 inline 实现。

#### report
- `README.md`：介绍日志系统架构。
- `include/lockfree_ringbuffer.hpp/.inl`：声明 SPSC ringbuffer 模板，用于日志缓冲。
- `include/log_backend.hpp`：声明后台线程接口。
- `include/logging.hpp`：对外日志宏与初始化 API。
- `include/logsteam.hpp`（typo 但保留文件名）：声明流式日志接口。
- `source/logging.cpp`：实现日志初始化/关闭。
- `source/log_backend.cpp`：实现后台消费线程。
- `source/logstream.cpp`：实现 `LogStream`，支持 `operator<<` 收集。
- `source/lockfree_ringbuffer.cpp`：提供 ringbuffer 的具体行为。

#### vocabulary
- `include/algorithm.hpp`：常用算法工具（静态数组拷贝等）。
- `include/fixed_position_container.hpp`：固定位置容器声明。
- `include/macros.hpp`：编译期/断言宏。
- `include/string.hpp`：定长 string 模板。
- `include/vector.hpp`：定长 vector 模板。
- `detail/fixed_position_container.inl`：对应实现。
- `detail/string.inl`：`ZeroCP::string` 内联实现。
- `detail/vector.inl`：`ZeroCP::vector` 内联实现。

---

上述两大目录的每个文件均已列出所属模块、当前职责与开发状态，可直接作为 ZeroCopy 2.0 的代码骨架与接口清单。后续在实现新特性时，建议在对应小节中补充实现状态或接口差异，保持文档与代码同步。


