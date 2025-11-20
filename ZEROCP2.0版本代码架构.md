## 目录结构

```text
zerocp/                                   # ZeroCP 项目根目录
├── CMakeLists.txt                        # 根构建入口，参考 iceoryx_posh 的模块拆分
├── zerocp_daemon/                        # 守护进程：控制面大脑 + 共享内存协调
│   ├── CMakeLists.txt                    # Daemon 模块构建脚本，聚合 application/control/ipc/shm 等子目标
│   ├── application/                      # 守护进程入口层：main + 配置 + 信号处理
│   │   ├── diroute_main.cpp              # 守护进程入口函数，初始化 Diroute 并进入主循环
│   │   └── daemon_config.hpp             # 守护进程配置结构与加载解析接口
│   │
│   ├── control/                          # 控制面核心：注册 → 心跳 → 路由 → 分发
│   │   ├── include/
│   │   │   ├── diroute.hpp               # Diroute 控制面总接口（run/stop/handleXxx）
│   │   │   ├── process_manager.hpp       # 进程注册表，管理 runtime/订阅列表/状态
│   │   │   ├── heartbeat_scheduler.hpp   # 心跳调度器接口，周期检查心跳超时
│   │   │   └── route_table.hpp           # 路由表接口，维护 service→subscriber 映射
│   │   └── source/
│   │       ├── diroute.cpp               # Diroute 实现：调用 ProcessManager/RouteTable/MemPool
│   │       ├── process_manager.cpp       # 进程注册/注销/查找/接收列表增删逻辑
│   │       ├── heartbeat_scheduler.cpp   # 心跳超时检测及进程清理逻辑
│   │       └── route_table.cpp           # service 级路由增删/查找/遍历实现
│   │
│   ├── ipc/                              # Daemon 侧 IPC：UDS server + 控制协议
│   │   ├── include/
│   │   │   ├── ipc_server.hpp            # UDS 服务器接口：监听/accept/事件回调
│   │   │   ├── routemsg.hpp              # 控制协议消息体定义（REGISTER/SUBSCRIBE 等）
│   │   │   ├── runtime_session.hpp       # 每个 runtime 连接的会话对象接口
│   │   │   └── service_description.hpp   # 服务三元组类型定义（service/instance/event）
│   │   └── source/
│   │       ├── ipc_server.cpp            # UDS 服务器实现，接收消息转交 Diroute
│   │       ├── routemsg.cpp              # 控制消息编解码与工具函数实现
│   │       ├── runtime_session.cpp       # 维护 runtime 会话状态与消息收发逻辑
│   │       └── service_description.cpp   # service 描述的比较/哈希/打印等实现
│   │
│   ├── shm/                              # 共享内存组件聚合（DirouteMemoryManager）
│   │   ├── include/
│   │   │   ├── diroute_components.hpp    # 共享段内组件聚合体（心跳池/端口表/入口指针等）
│   │   │   ├── diroute_memory_manager.hpp# 管理 /zerocp_mgmt 映射与组件构造/访问接口
│   │   │   ├── heartbeat_pool.hpp        # HeartbeatPool 接口，管理所有 runtime 心跳槽
│   │   │   └── port_registry.hpp         # PortRegistry 接口，记录 pub/sub 端口元信息
│   │   └── source/
│   │       ├── diroute_memory_manager.cpp# DirouteMemoryManager 实现（创建/attach 共享内存）
│   │       ├── heartbeat_pool.cpp        # HeartbeatPool 槽分配/释放/遍历实现
│   │       └── port_registry.cpp         # 注册/查找/删除端口及其句柄实现
│   │
│   ├── mempool/                          # chunk 级内存池与 SharedChunk 生命周期
│   │   ├── CROSS_PROCESS_TRANSFER.md     # 文档：跨进程传递 chunk 句柄的约束与规则
│   │   ├── SHARED_CHUNK_USAGE.md         # 文档：SharedChunk 生命周期与使用示例
│   │   ├── include/
│   │   │   ├── mempool_config.hpp        # mempool 配置结构：size class/数量/默认集
│   │   │   ├── mempool_allocator.hpp     # 在共享段内划分管理区/数据区的布局构造器接口
│   │   │   ├── mempool.hpp               # 单个 mempool 类型，管理一组 chunk header + data
│   │   │   ├── mempool_manager.hpp       # MemPoolManager 接口：创建/挂载/loan/release
│   │   │   ├── shared_chunk.hpp          # SharedChunk RAII 封装，管理 chunk 引用计数
│   │   │   └── chunk_header.hpp          # chunk header 定义（尺寸/offset/refcount 等）
│   │   └── source/
│   │       ├── mempool_config.cpp        # mempool 配置增删/默认集生成实现
│   │       ├── mempool_allocator.cpp     # 共享内存 mempool 布局分配实现
│   │       ├── mempool.cpp               # mempool 初始化与 raw memory 绑定实现
│   │       ├── mempool_manager.cpp       # 多个 mempool 的管理与 chunk loan/release 实现
│   │       ├── shared_chunk.cpp          # SharedChunk attach/release/refcount 操作实现
│   │       └── memory_layout_docs/       # 内存布局相关文档目录
│   │           ├── memory.md             # mempool 内存设计要点文档
│   │           └── shared_memory_layout_detailed.md # 共享段详细布局说明
│   │
│   └── introspection/                    # 监控/调试（可选）
│       ├── include/
│       │   ├── mempool_introspection.hpp # mempool 状态统计/导出接口
│       │   ├── process_introspection.hpp # 进程/订阅列表状态查询接口
│       │   └── heartbeat_introspection.hpp# 心跳槽状态/超时统计接口
│       └── source/
│           ├── mempool_introspection.cpp # 导出 mempool 使用情况/统计信息实现
│           ├── process_introspection.cpp # 导出进程列表/订阅列表信息实现
│           └── heartbeat_introspection.cpp# 导出心跳信息/监控指标实现
│
├── zerocp_runtime/                       # 应用侧 Runtime：pub/sub API + 控制通道客户端
│   ├── CMakeLists.txt                    # Runtime 模块构建脚本，聚合 api/ipc/shmclient/lifecycle
│   ├── api/                              # 对用户暴露的高层 C++ API
│   │   ├── include/
│   │   │   ├── zerocp/runtime.hpp        # Runtime 单例接口：init/getInstance/createPublisher/Subscriber
│   │   │   ├── zerocp/service_description.hpp # Runtime 侧 service 描述类型声明
│   │   │   ├── zerocp/publisher.hpp      # ZeroPublisher<T> 模板接口（loan/publish）
│   │   │   ├── zerocp/subscriber.hpp     # ZeroSubscriber<T> 模板接口（subscribe/take）
│   │   │   ├── zerocp/loaned_sample.hpp  # LoanedSample<T>，封装单个 chunk 的 RAII 句柄
│   │   │   └── zerocp/config.hpp         # runtime 初始化配置（UDS 名、心跳周期等）
│   │   └── source/
│   │       ├── runtime.cpp               # Runtime 单例实现，整合 ipc/shmclient/lifecycle
│   │       ├── publisher.cpp             # Publisher 非模板部分实现（工厂/公共逻辑）
│   │       └── subscriber.cpp            # Subscriber 非模板部分实现（订阅控制等）
│   │
│   ├── ipc/                              # Runtime ↔ Daemon 控制通道客户端（UDS client）
│   │   ├── include/
│   │   │   ├── ipc_runtime_interface.hpp # 统一 IPC 接口：连接/发送控制消息/接收响应
│   │   │   ├── ipc_interface_creator.hpp # UDS client 构造器（封装 socket 创建/连接）
│   │   │   ├── routemsg_codec.hpp        # 控制消息编码/解码工具（与 daemon 协议对应）
│   │   │   └── message_runtime.hpp       # 本 runtime 元信息（pid/uid/appName 等）
│   │   └── source/
│   │       ├── ipc_runtime_interface.cpp # IPC 接口默认实现（基于 Unix Domain Socket）
│   │       ├── ipc_interface_creator.cpp # UDS client builder 实现
│   │       ├── routemsg_codec.cpp        # 控制消息序列化/反序列化实现
│   │       └── message_runtime.cpp       # MessageRuntime 构造与字段访问实现
│   │
│   ├── shmclient/                        # 共享内存客户端（数据面 attach）
│   │   ├── include/
│   │   │   ├── shm_client.hpp            # 映射 daemon 创建的共享段，提供地址解析接口
│   │   │   ├── chunk_allocator_client.hpp# 访问 mempool 的轻量 allocator 客户端封装
│   │   │   ├── chunk_queue.hpp           # 单个 subscriber 的 chunk 队列（ChunkHeader* FIFO）
│   │   │   └── heartbeat_slot.hpp        # runtime 在 HeartbeatPool 中的心跳槽访问封装
│   │   └── source/
│   │       ├── shm_client.cpp            # shm_client attach/detach 与 offset 解析实现
│   │       ├── chunk_allocator_client.cpp# 调用 MemPoolManager loan/release 的客户端实现
│   │       ├── chunk_queue.cpp           # chunk 队列 push/pop 等逻辑实现
│   │       └── heartbeat_slot.cpp        # 心跳槽写入/标记更新实现
│   │
│   └── lifecycle/                        # Runtime 进程生命周期（注册/心跳/注销）
│       ├── include/
│       │   ├── processinfo.hpp           # 本进程状态描述结构（名称/角色/配置等）
│       │   ├── process_lifecycle.hpp     # 生命周期管理接口：注册→心跳→注销
│       │   └── heartbeat_thread.hpp      # 心跳线程封装，定期更新 HeartbeatPool
│       └── source/
│           ├── process_lifecycle.cpp     # 注册/注销/异常退出处理逻辑实现
│           ├── heartbeat_thread.cpp      # 心跳线程创建/循环/关闭实现
│           └── processinfo.cpp           # processinfo 字段访问与辅助函数实现
│
└── zerocp_foundationLib/                 # 基础设施：POSIX 封装、内存/并发、日志、固定容器
    ├── CMakeLists.txt                    # FoundationLib 构建脚本，导出 concurrent/memory/report/vocabulary 等静态库
    ├── concurrent/                       # 并发工具：freelist + 并发规范
    │   ├── README.md                     # 文档：MPMC lock-free freelist 的设计与用法
    │   ├── include/
    │   │   └── mpmclockfreelist.hpp      # MPMC lock-free freelist 模板声明（push/pop/init）
    │   ├── sysc/                         # 并发规范：atomic/smart_lock 封装（可视为 sync 语义）
    │   │   ├── include/
    │   │   │   ├── atomic_design.hpp     # atomic 封装设计规范（统一原子 API 与内存序）
    │   │   │   └── smart_lock.hpp        # smart_lock 设计规范（RAII 包裹互斥锁访问）
    │   │   └── detail/
    │   │       ├── atomic_design.inl     # atomic_design 具体内联实现示例
    │   │       └── smart_lock.inl        # smart_lock 内联实现示例
    │   └── source/
    │       └── mpmclockfreelist.cpp      # freelist 实现，处理 ABA 计数与节点初始化
    │
    ├── core/                             # 通用核心接口（expected/utility 等，预留）
    │   └── include/
    │       └── (占位，待扩展)             # 预留 future 核心工具接口位置
    │
    ├── design/                           # Builder 模式、命名规范
    │   ├── builder.hpp                   # 通用 Builder 模式模板，支持链式配置
    │   └── linux_name.hpp                # shm/uds 名称规范与校验工具
    │
    ├── filesystem/                       # 文件系统工具/可替换接口
    │   ├── include/
    │   │   ├── filesystem.hpp            # 文件系统辅助函数集合声明（路径/目录操作等）
    │   │   └── filesystem_interface.hpp  # 可替换的文件系统接口抽象定义
    │   └── source/
    │       └── filesystem_interface.cpp  # 默认文件系统接口实现（基于 POSIX 调用）
    │
    ├── memory/                           # 基础内存工具 & BumpAllocator
    │   ├── include/
    │   │   ├── bump_allocator.hpp        # BumpAllocator 声明，用于共享段一次性布局
    │   │   └── memory.hpp                # 对齐/分配工具函数声明
    │   └── source/
    │       ├── bump_allocator.cpp        # 连续分配/重置与错误处理实现
    │       └── memory.cpp                # align/alignedAlloc 等基础工具实现
    │
    ├── posix/                            # POSIX 封装：shm/mmap/UDS/系统调用
    │   ├── memory/
    │   │   ├── LOGGING_GUIDE.md          # 文档：POSIX 内存组件的日志规范
    │   │   ├── RELATIVE_POINTER_USAGE.md # 文档：relative pointer 使用注意事项
    │   │   ├── detail/
    │   │   │   └── relative_pointer.inl  # relative pointer 内联实现细节
    │   │   ├── include/
    │   │   │   ├── posix_memorymap.hpp   # mmap 封装 builder/对象接口声明
    │   │   │   ├── posix_sharedmemory.hpp# shm_open/ftruncate 等共享内存接口声明
    │   │   │   ├── posix_sharedmemory_object.hpp # 组合 shm+memorymap 的高层对象
    │   │   │   └── relative_pointer.hpp  # offset-based 相对指针工具声明
    │   │   └── source/
    │   │       ├── posix_memorymap.cpp   # 内存映射封装具体实现
    │   │       ├── posix_sharedmemory.cpp# 共享内存创建/打开/销毁实现
    │   │       └── posix_sharedmemory_object.cpp # 组合对象构造与访问实现
    │   │
    │   ├── ipc/
    │   │   ├── include/
    │   │   │   └── unix_domainsocket.hpp # Unix Domain Socket 封装接口（builder+对象）
    │   │   └── source/
    │   │       └── unix_domainsocket.cpp # UDS 创建/绑定/连接/收发实现
    │   │
    │   └── posixcall/
    │       ├── include/
    │       │   └── posix_call.hpp        # 对 read/write/open 等 POSIX 调用的包装声明
    │       └── detail/
    │           └── posix_call.inl        # 内联实现，统一 errno 处理与日志
    │
    ├── report/                           # 日志系统：lock-free ringbuffer + backend
    │   ├── README.md                     # 日志系统架构与使用说明
    │   ├── include/
    │   │   ├── lockfree_ringbuffer.hpp   # SPSC lock-free ringbuffer 模板声明（日志缓冲）
    │   │   ├── log_backend.hpp           # 日志后台线程接口声明
    │   │   ├── logging.hpp               # 对外日志初始化/宏接口
    │   │   └── logstream.hpp             # 流式日志接口声明（operator<< 收集）
    │   └── source/
    │       ├── lockfree_ringbuffer.cpp   # 环形缓冲区具体实现
    │       ├── log_backend.cpp           # 后台日志消费线程实现
    │       ├── logging.cpp               # 日志系统初始化/关闭/全局配置实现
    │       └── logstream.cpp             # LogStream 细节实现
    │
    └── vocabulary/                       # 固定容器 & 常用算法
        ├── include/
        │   ├── algorithm.hpp             # 常用小算法工具（静态数组拷贝等）
        │   ├── fixed_position_container.hpp # 固定位置容器声明（适用于心跳池等固定槽位）
        │   ├── macros.hpp                # 编译期/断言/平台宏定义
        │   ├── string.hpp                # 定长 string 模板类型声明
        │   └── vector.hpp                # 定长 vector 模板类型声明
        └── detail/
            ├── fixed_position_container.inl # fixed_position_container 内联实现
            ├── string.inl                 # ZeroCP::string 内联实现
            └── vector.inl                 # ZeroCP::vector 内联实现
```

## 总结

- 控制面（`zerocp_daemon`）聚焦注册、路由、共享内存治理，并通过 `introspection` 输出观测数据，为策略插件留扩展点。
- Runtime 侧（`zerocp_runtime`）以 `Runtime::init → process_lifecycle → heartbeat_thread` 串起注册、共享内存 attach、心跳维持的完整链路，保证应用进程与 daemon 的一致视图。
- 基础库（`zerocp_foundationLib`）提供 POSIX/内存/容器/日志等零拷贝基石，确保控制面与 Runtime 能复用统一的 builder、容器与诊断设施。
- 行动层面需保持文档-代码镜像、补齐控制面/基础层测试、增强 introspection 与插件化能力，为 ZeroCP 2.0 的可运维与可扩展打底。