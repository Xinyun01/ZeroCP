# ZeroCp POSIX Call Framework 测试套件

本目录包含 ZeroCp POSIX 调用框架的完整测试套件。

## 📁 目录结构

```
zerocp_tests/posix_call/
├── test_posix_call.cpp    # 主测试文件
├── CMakeLists.txt         # CMake 构建配置
├── run_test.sh            # 测试运行脚本
└── README.md              # 本文档
```

## 🚀 快速开始

### 方法 1: 使用脚本运行（推荐）

```bash
# 给脚本添加执行权限
chmod +x run_test.sh

# 运行测试
./run_test.sh
```

### 方法 2: 手动编译运行

```bash
# 创建构建目录
mkdir -p build
cd build

# 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 编译
make -j$(nproc)

# 运行测试
./test_posix_call
```

## 📋 测试覆盖

### 基础功能测试

1. **Open with failure value** - 测试 `open()` 使用 `failureReturnValue(-1)`
2. **Open failure case** - 测试 `open()` 失败情况
3. **Close with success value** - 测试 `close()` 使用 `successReturnValue(0)`
4. **Close failure case** - 测试 `close()` 失败情况
5. **Write operation** - 测试 `write()` 操作
6. **Read operation** - 测试 `read()` 操作
7. **Lseek operation** - 测试 `lseek()` 文件定位
8. **Unlink operation** - 测试 `unlink()` 删除文件

### 高级功能测试

9. **Multiple success values** - 测试可变参数的成功值
10. **Multiple failure values** - 测试可变参数的失败值
11. **Complete workflow** - 测试完整的文件操作流程
12. **Zero byte write** - 测试边界条件（零字节写入）
13. **Type deduction** - 测试不同返回类型的类型推导

## ✅ 验证项

每个测试都会验证以下内容：

- ✓ 调用成功/失败的正确判断
- ✓ 返回值的正确性
- ✓ 错误码（errno）的正确保存
- ✓ 类型安全和类型推导
- ✓ 链式调用的流畅性
- ✓ 边界条件处理

## 📊 测试输出示例

```
========================================
  ZeroCp POSIX Call Framework Tests
========================================

[TEST] Open with failure value
  ✓ PASSED: eval.hasSuccess()
  ✓ PASSED: (fd) >= (0)
  ✓ PASSED: (eval.getErrnum()) == (0)
✅ Open with failure value - PASSED

[TEST] Open failure case
  ✓ PASSED: !(eval.hasSuccess())
  ✓ PASSED: (eval.getValue()) == (-1)
  ✓ PASSED: (eval.getErrnum()) != (0)
  ℹ Error: errno=2 (No such file or directory)
✅ Open failure case - PASSED

...

========================================
  Test Summary
========================================
Total:  13
Passed: 13 ✅
Failed: 0 ❌
========================================
```

## 🔧 开发指南

### 添加新测试

1. 在 `test_posix_call.cpp` 中添加测试函数：

```cpp
bool test_my_new_feature() {
    // 测试逻辑
    ASSERT_TRUE(condition);
    return true;
}
```

2. 在 `main()` 函数中注册测试：

```cpp
runner.addTest("My new feature", test_my_new_feature);
```

### 使用断言宏

```cpp
ASSERT_TRUE(condition)   // 断言条件为真
ASSERT_FALSE(condition)  // 断言条件为假
ASSERT_EQ(a, b)         // 断言 a == b
ASSERT_NE(a, b)         // 断言 a != b
ASSERT_GE(a, b)         // 断言 a >= b
ASSERT_LE(a, b)         // 断言 a <= b
```

## 📝 注意事项

1. 测试使用 `/tmp/zerocp_posix_test.txt` 作为临时测试文件
2. 所有测试完成后会自动清理临时文件
3. 测试需要读写 `/tmp` 目录的权限
4. 某些失败测试是故意的，用于验证错误处理

## 🐛 调试

如果测试失败，可以使用以下方法调试：

```bash
# 使用 Debug 模式编译
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 使用 GDB 调试
gdb ./test_posix_call

# 使用 Valgrind 检查内存
valgrind --leak-check=full ./test_posix_call
```

## 📚 相关文档

- [POSIX Call Framework 头文件](../../zerocp_foundationLib/posix/system_call/include/posix_call.hpp)
- [项目主 README](../../README.md)
- [代码规范](../../CODE_STYLE.md)

## 📧 反馈

如果发现问题或有改进建议，请联系 ZeroCp 团队。

