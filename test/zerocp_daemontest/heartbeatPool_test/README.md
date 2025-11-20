# 心跳池注册与回收测试摘要

该目录用于验证 `diroute_main` 在共享内存心跳池中的注册、监控与回收逻辑。核心场景是：一次性拉起 10 个测试客户端，逐个终止并观察守护进程是否在日志中检测到超时、释放槽位并最终归零。

## 目录结构概览

- 位置：`/home/xinyun/Infrastructure/zero_copy_framework/test/zerocp_daemontest/heartbeatPool_test`
- 关键文件：
  - `client_base.hpp`、`client_0.cpp ~ client_9.cpp`：10 个示例客户端
  - `diagnose_heartbeat.cpp`：辅助诊断工具
  - `build_and_test.sh`：一键构建 + 运行脚本
  - `test_slot_index_timeout.sh`：默认顺序回收测试
  - `cleanup.sh`：收尾、清理共享内存与日志

## 快速使用

1. **准备二进制**
   ```bash
   # 构建守护进程
   cd /home/xinyun/Infrastructure/zero_copy_framework/zerocp_daemon
   cmake -S . -B build && cmake --build build

   # 构建测试客户端
   cd /home/xinyun/Infrastructure/zero_copy_framework/test/zerocp_daemontest/heartbeatPool_test
   ./build_and_test.sh --build-only
   ```

2. **运行验证**
   ```bash
   ./build_and_test.sh   # 自动调用 test_slot_index_timeout.sh
   ```
   - 守护进程：`zerocp_daemon/build/bin/diroute_main`
   - 客户端：`build/bin/client_0 ~ client_9`
   - 守护进程每秒打印注册统计，客户端每 100ms 写心跳。

3. **观察日志**
   ```bash
   tail -f daemon.log | grep -E "Registered|Process timeout|Total registered"
   ```
   关注：注册总数达到 10、检测超时、释放槽位、最终归零后 `SIGUSR1` 触发的注册列表打印。

4. **人工干预（可选）**
   ```bash
   pkill -f client_5   # 或 kill -TERM <PID>
   sleep 3
   tail -n 20 daemon.log
   ```
   期望 3 秒内出现 `Process timeout detected` 与 `Remaining registered processes` 递减。

5. **收尾清理**
   ```bash
   ./cleanup.sh --logs
   ```
   终止残留进程并删除 `/dev/shm/zerocp_*`、`.sock` 与日志。

## 常用排查命令

```bash
grep "Registered Processes" daemon.log          # 快速看注册数
grep "Heartbeat thread running" client_*.log    # 客户端心跳状态
ps -ef | grep client_                           # 查询客户端 PID
pkill -f "client_[0-9]"                         # 一次性终止所有客户端
```

## 验收标准

1. 日志中出现 10 个槽位注册完成；
2. 任意客户端被 kill 后 3 秒内检测到超时并释放槽位；
3. 客户端可重启并成功重新注册；
4. `cleanup.sh` 执行后无残留进程、共享内存或 UDS 文件。

按照以上步骤即可完成“10 个应用注册 → 守护进程监控 → 逐个终止并验证回收”的完整测试闭环。
