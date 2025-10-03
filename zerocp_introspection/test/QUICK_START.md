# 测试快速开始指南

## 🚀 快速运行测试

### 1. 编译测试

```bash
cd /home/xinyun/Infrastructure/zero_copy_framework

# 如果 build 目录不存在
mkdir -p build && cd build

# 配置并编译
cmake -DBUILD_INTROSPECTION_TESTS=ON ..
make introspection_tests -j$(nproc)
```

### 2. 运行所有测试

```bash
# 在 build 目录中
./bin/introspection_tests
```

### 3. 预期输出

```
[==========] Running 60+ tests from 4 test suites.
[----------] Global test environment set-up.
...
[----------] 12 tests from IntrospectionTypesTest
[ RUN      ] IntrospectionTypesTest.MemoryInfoBasic
[       OK ] IntrospectionTypesTest.MemoryInfoBasic (0 ms)
...
[==========] 60+ tests from 4 test suites ran. (XXXX ms total)
[  PASSED  ] 60+ tests.
```

## 📋 测试文件说明

| 文件 | 测试数量 | 描述 |
|------|---------|------|
| `test_introspection_types.cpp` | ~12 | 数据类型和结构体 |
| `test_introspection_server.cpp` | ~15 | 服务端功能 |
| `test_introspection_client.cpp` | ~20 | 客户端功能 |
| `test_integration.cpp` | ~12 | 集成和端到端测试 |

## 🎯 常用测试命令

### 运行特定测试套件

```bash
# 只测试服务端
./bin/introspection_tests --gtest_filter=IntrospectionServerTest.*

# 只测试客户端
./bin/introspection_tests --gtest_filter=IntrospectionClientTest.*

# 只测试类型
./bin/introspection_tests --gtest_filter=IntrospectionTypesTest.*

# 只测试集成
./bin/introspection_tests --gtest_filter=IntegrationTest.*
```

### 运行特定测试用例

```bash
# 测试启动和停止
./bin/introspection_tests --gtest_filter=*.StartAndStop

# 测试连接功能
./bin/introspection_tests --gtest_filter=*Connect*

# 测试订阅功能
./bin/introspection_tests --gtest_filter=*Subscribe*
```

### 调试选项

```bash
# 详细输出
./bin/introspection_tests --gtest_verbose

# 显示测试耗时
./bin/introspection_tests --gtest_print_time=1

# 失败后立即停止
./bin/introspection_tests --gtest_break_on_failure

# 重复运行（检测不稳定的测试）
./bin/introspection_tests --gtest_repeat=10
```

## 📊 测试覆盖的功能

### ✅ 服务端 (Server)
- [x] 启动/停止/重启
- [x] 配置获取和更新
- [x] 数据收集（周期性和立即）
- [x] 回调注册和通知
- [x] 进程和连接过滤
- [x] 多客户端支持
- [x] 线程安全
- [x] 并发访问

### ✅ 客户端 (Client)
- [x] 连接/断开/重连
- [x] 获取完整指标
- [x] 获取内存信息
- [x] 获取进程列表
- [x] 获取连接列表
- [x] 获取负载信息
- [x] 获取配置
- [x] 请求配置更新
- [x] 请求立即收集
- [x] 事件订阅/取消订阅

### ✅ 集成测试
- [x] 完整工作流程
- [x] 多客户端场景
- [x] 事件广播
- [x] 动态配置更新
- [x] 过滤功能
- [x] 长时间运行稳定性
- [x] 客户端重连
- [x] 服务器重启
- [x] 异常处理

## 🔧 故障排查

### 编译失败

**问题**: Google Test 下载失败
```
解决方案：
1. 检查网络连接
2. 或手动安装 Google Test:
   sudo apt-get install libgtest-dev
```

**问题**: 找不到头文件
```
解决方案：
确保在正确的目录运行 cmake
cd /home/xinyun/Infrastructure/zero_copy_framework/build
cmake ..
```

### 测试失败

**问题**: 时间相关测试偶尔失败
```
原因: 系统负载高导致延迟
解决方案: 正常现象，重新运行测试
```

**问题**: 进程过滤测试失败
```
原因: 系统中没有被过滤的进程
解决方案: 正常现象，测试会适配
```

### 运行失败

**问题**: 权限不足
```bash
# 确保可执行权限
chmod +x ./bin/introspection_tests
```

**问题**: 找不到共享库
```bash
# 检查库路径
ldd ./bin/introspection_tests
```

## 📈 测试性能基准

在典型的开发机器上（4核CPU，8GB RAM）：

- **总测试时间**: ~30-60 秒
- **单个测试**: 通常 < 100ms
- **集成测试**: 可能需要数秒（包含等待时间）

## 🎓 添加自己的测试

### 1. 创建测试文件

```cpp
// test_my_feature.cpp
#include <gtest/gtest.h>
#include "introspection/introspection_server.hpp"

TEST(MyFeatureTest, BasicTest) {
    // 测试代码
    EXPECT_TRUE(true);
}
```

### 2. 更新 CMakeLists.txt

```cmake
set(TEST_SOURCES
    test_introspection_types.cpp
    test_introspection_server.cpp
    test_introspection_client.cpp
    test_integration.cpp
    test_my_feature.cpp  # 添加你的测试
)
```

### 3. 重新编译

```bash
cd build
make introspection_tests
./bin/introspection_tests --gtest_filter=MyFeatureTest.*
```

## 📝 测试编写建议

1. **使用有意义的测试名称**
   ```cpp
   TEST(ServerTest, StartSucceedsWithValidConfig)  // 好
   TEST(ServerTest, Test1)                          // 不好
   ```

2. **使用 Test Fixture 共享设置**
   ```cpp
   class MyTest : public ::testing::Test {
   protected:
       void SetUp() override { /* 初始化 */ }
       void TearDown() override { /* 清理 */ }
   };
   ```

3. **测试成功和失败路径**
   ```cpp
   TEST(ServerTest, StartSucceeds) { /* ... */ }
   TEST(ServerTest, StartFailsWhenAlreadyRunning) { /* ... */ }
   ```

4. **使用适当的断言**
   ```cpp
   EXPECT_EQ(a, b);     // 相等
   EXPECT_NE(a, b);     // 不相等
   EXPECT_GT(a, b);     // 大于
   EXPECT_TRUE(cond);   // 布尔值
   EXPECT_THROW(expr, exception_type);  // 异常
   ```

## 🚦 CI/CD 集成示例

```bash
#!/bin/bash
# run_tests.sh

set -e

echo "Building tests..."
mkdir -p build && cd build
cmake -DBUILD_INTROSPECTION_TESTS=ON ..
make introspection_tests -j$(nproc)

echo "Running tests..."
./bin/introspection_tests --gtest_output=xml:test_results.xml

echo "Tests passed!"
```

## 📞 获取帮助

```bash
# 显示所有可用选项
./bin/introspection_tests --help

# 列出所有测试
./bin/introspection_tests --gtest_list_tests
```

## ✨ 下一步

- 查看 `README.md` 了解详细测试说明
- 查看 `../ARCHITECTURE.md` 了解组件架构
- 查看 `../example_usage.cpp` 了解使用示例
- 运行 `./bin/introspection` 体验监控工具

祝测试愉快！🎉

