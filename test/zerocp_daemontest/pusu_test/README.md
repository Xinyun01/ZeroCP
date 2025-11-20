# PUSU 集成测试

本目录提供一个最小可复现的 Publisher/Subscriber 集成测试，用于验证：

- `diroute_main` 能够初始化共享内存与心跳池
- `PoshRuntime` 注册流程（REGISTER / PUBLISHER / SUBSCRIBER）
- `Publisher` 零拷贝 loan/publish 流程
- `Subscriber` 接收队列映射以及 `take()` 读取

## 构建

```bash
cd /home/xinyun/Infrastructure/zero_copy_framework/test/zerocp_daemontest/pusu_test
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target diroute_main pusu_publisher pusu_subscriber -j$(nproc)
```

可执行文件位于 `build/bin/`。

## 运行

推荐使用 `run_pusu_test.sh`，它会：

1. 清理残留 socket 与共享内存
2. 启动 `diroute_main`
3. 启动订阅者，再启动发布者
4. 等待订阅者接收固定数量的样本

```bash
./run_pusu_test.sh
```

若需要手动运行：

```bash
build/bin/diroute_main &
diroute_pid=$!
sleep 1
build/bin/pusu_subscriber > subscriber.log &
sub_pid=$!
sleep 1
build/bin/pusu_publisher > publisher.log
wait $sub_pid
kill -TERM $diroute_pid
```

## 清理

测试结束可执行 `cleanup.sh` 删除 `/dev/shm/zerocp_*`、残留 sock 文件以及运行中的测试进程。

