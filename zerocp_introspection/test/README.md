# Introspection 组件测试

## 测试概览

测试套件包含全面的单元测试和集成测试，确保 introspection 组件的正确性和稳定性。

## 测试文件

### 1. `test_introspection_types.cpp`
测试所有数据类型和结构体：
- `MemoryInfo` - 内存信息
- `ProcessInfo` - 进程信息
- `ConnectionInfo` - 连接信息
- `LoadInfo` - 系统负载信息
- `SystemMetrics` - 系统指标
- `IntrospectionConfig` - 配置
- `IntrospectionEvent` - 事件

**测试内容**：
- 基本构造和赋值
- 数据有效性验证
- 拷贝和移动语义
- 大数值处理

### 2. `test_introspection_server.cpp`
测试 `IntrospectionServer` 类：
- 启动和停止
- 配置管理
- 数据收集
- 回调注册和通知
- 过滤功能
- 线程安全

**测试内容**：
- 基本生命周期（启动/停止/重启）
- 配置获取和更新
- 立即数据收集
- 单/多回调注册
- 进程和连接过滤
- 并发访问安全性
- 异常处理

### 3. `test_introspection_client.cpp`
测试 `IntrospectionClient` 类：
- 连接管理
- 同步查询
- 异步订阅
- 配置请求
- 多客户端支持

**测试内容**：
- 连接/断开/重连
- 各种数据查询接口
- 事件订阅和取消订阅
- 配置更新请求
- 立即收集请求
- 多客户端并发访问
- 客户端生命周期

### 4. `test_integration.cpp`
集成测试：
- 完整工作流程
- 多客户端场景
- 事件广播
- 动态配置
- 长时间运行稳定性

**测试内容**：
- 端到端工作流程
- 多客户端协作
- 事件广播机制
- 动态配置更新
- 过滤功能集成
- 长时间运行测试
- 客户端重连
- 服务器重启
- 并发配置更新
- 异常处理

## 编译和运行测试

### 编译测试

```bash
cd build
cmake -DBUILD_INTROSPECTION_TESTS=ON ..
make introspection_tests
```

### 运行所有测试

```bash
./bin/introspection_tests
```

### 运行特定测试

```bash
# 运行特定测试套件
./bin/introspection_tests --gtest_filter=IntrospectionServerTest.*

# 运行特定测试用例
./bin/introspection_tests --gtest_filter=IntrospectionServerTest.StartAndStop

# 运行多个测试
./bin/introspection_tests --gtest_filter=*Server*:*Client*
```

### 详细输出

```bash
# 显示详细输出
./bin/introspection_tests --gtest_verbose

# 显示测试进度
./bin/introspection_tests --gtest_print_time=1
```

### 重复运行测试

```bash
# 重复运行 10 次
./bin/introspection_tests --gtest_repeat=10

# 失败后立即停止
./bin/introspection_tests --gtest_repeat=100 --gtest_break_on_failure
```

## 测试覆盖率

测试覆盖了以下关键功能：

### 服务端 (IntrospectionServer)
- ✅ 启动/停止/重启
- ✅ 配置管理
- ✅ 数据收集（周期性和立即）
- ✅ 回调注册和通知
- ✅ 进程/连接过滤
- ✅ 线程安全和并发访问
- ✅ 异常处理

### 客户端 (IntrospectionClient)
- ✅ 连接管理
- ✅ 同步查询（所有数据类型）
- ✅ 异步订阅
- ✅ 配置请求
- ✅ 立即收集请求
- ✅ 多客户端支持
- ✅ 重连机制

### 集成场景
- ✅ 完整工作流程
- ✅ 多客户端协作
- ✅ 事件广播
- ✅ 动态配置更新
- ✅ 长时间运行稳定性
- ✅ 内存泄漏检测
- ✅ 异常处理

## 测试统计

| 测试类别 | 测试数量 | 描述 |
|---------|---------|------|
| 类型测试 | 12+ | 数据结构和类型 |
| 服务端测试 | 15+ | 服务端功能 |
| 客户端测试 | 20+ | 客户端功能 |
| 集成测试 | 12+ | 端到端场景 |
| **总计** | **60+** | 全面覆盖 |

## 性能测试

一些测试会评估性能特征：

- **数据收集延迟**：通常 < 100ms
- **事件通知延迟**：配置的更新间隔 ± 50ms
- **查询延迟**：< 1ms (从缓存读取)
- **并发访问**：支持多个客户端同时访问

## 调试技巧

### 1. 单步调试特定测试

```bash
gdb --args ./bin/introspection_tests --gtest_filter=IntegrationTest.CompleteWorkflow
```

### 2. 添加调试输出

在测试中使用 `std::cout` 或 `GTEST_LOG_(INFO)`:

```cpp
TEST_F(MyTest, MyTestCase) {
    GTEST_LOG_(INFO) << "Starting test...";
    // 测试代码
}
```

### 3. 检查失败的测试

```bash
# 只运行失败的测试
./bin/introspection_tests --gtest_also_run_disabled_tests --gtest_filter=*DISABLED_*
```

## CI/CD 集成

### GitHub Actions 示例

```yaml
- name: Build and Test
  run: |
    mkdir build && cd build
    cmake -DBUILD_INTROSPECTION_TESTS=ON ..
    make introspection_tests
    ./bin/introspection_tests --gtest_output=xml:test_results.xml
```

### 输出 XML 报告

```bash
./bin/introspection_tests --gtest_output=xml:test_results.xml
```

## 添加新测试

### 1. 创建新的测试文件

```cpp
#include <gtest/gtest.h>
#include "introspection/introspection_types.hpp"

TEST(MyTestSuite, MyTestCase) {
    // 测试代码
    EXPECT_EQ(1, 1);
}
```

### 2. 更新 CMakeLists.txt

```cmake
set(TEST_SOURCES
    # ... 现有文件
    test_my_new_feature.cpp  # 添加新文件
)
```

### 3. 重新编译

```bash
cd build
make introspection_tests
./bin/introspection_tests
```

## 测试最佳实践

1. **使用 Fixture**：为相关测试创建 Test Fixture
2. **清理资源**：在 `TearDown()` 中确保资源清理
3. **独立测试**：每个测试应该独立，不依赖其他测试
4. **有意义的名称**：测试名称应该清晰描述测试内容
5. **验证所有分支**：测试成功和失败路径
6. **并发测试**：测试线程安全和并发场景
7. **边界条件**：测试边界值和极端情况

## 故障排查

### 测试失败
1. 检查测试输出，查看失败原因
2. 使用 `--gtest_filter` 单独运行失败的测试
3. 添加调试输出定位问题
4. 使用 gdb 进行调试

### 测试超时
1. 检查是否有死锁
2. 验证线程是否正确退出
3. 增加超时时间（如果合理）

### 随机失败
1. 检查时间相关的断言（允许适当误差）
2. 验证线程同步是否正确
3. 使用 `--gtest_repeat` 重复运行

## 依赖

测试依赖：
- Google Test (自动下载)
- pthread
- C++17 编译器

## 贡献指南

添加新功能时：
1. 编写对应的单元测试
2. 添加集成测试（如果适用）
3. 确保所有测试通过
4. 更新此 README（如果需要）

## 许可证

与主项目相同

