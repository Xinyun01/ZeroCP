# ZeroCP String 测试

本目录包含 ZeroCP 固定容量字符串类 (`string<Capacity>`) 的测试用例。

## 特性

### 字符串类特性
- ✅ **栈上分配**：完全在栈上分配，无堆内存使用
- ✅ **固定容量**：编译时指定容量，`string<Capacity>`
- ✅ **零拷贝设计**：优化的内存操作
- ✅ **编译时检查**：字符串字面量的编译时容量检查
- ✅ **运行时检查**：动态字符串的运行时容量检查
- ✅ **日志集成**：容量溢出时自动记录错误日志

### 测试覆盖

#### test_string.cpp - 综合功能测试
1. **基本栈上字符串创建和输出**
2. **字符串插入操作**（栈上输入）
3. **拷贝构造**（栈到栈）
4. **移动构造**（栈到栈）
5. **赋值操作符**（拷贝和移动）
6. **清空操作**
7. **不同容量的字符串**
8. **验证栈分配**（通过地址）
9. **编译时和运行时容量检查**
   - 字符串字面量的编译时检查
   - 运行时容量不足处理
   - const char* 指针的运行时检查
   - 中间位置插入
   - 边界条件测试

#### test_overflow.cpp - 容量溢出与日志测试
- 运行时溢出（使用 const char*）
- 运行时溢出（使用字符串字面量）
- 验证日志输出

## 构建和运行

### 使用 CMake

```bash
cd build
cmake ..
make

# 运行所有测试
./test_string

# 运行溢出测试
./test_overflow
```

### 使用快捷脚本

```bash
# 简单构建
./simple_build.sh

# 构建并运行（包含彩色输出）
./build_and_run.sh
```

## 核心 API

### 构造函数
- `string()` - 默认构造，创建空字符串
- `string(const string& other)` - 拷贝构造
- `string(string&& other)` - 移动构造

### 基本操作
- `uint64_t size() const` - 获取当前大小
- `uint64_t capacity() const` - 获取容量
- `const char* c_str() const` - 获取 C 风格字符串
- `bool empty() const` - 检查是否为空
- `void clear()` - 清空字符串

### 插入操作
```cpp
// 运行时检查版本（接受 const char* 指针）
string& insert(uint64_t pos, const char* str);

// 编译时检查版本（接受字符串字面量）
template<uint64_t N>
string& insert(uint64_t pos, const char (&str)[N]);
```

## 编译时 vs 运行时检查

### 编译时检查
当使用字符串字面量时，编译器可以在编译时检查字符串长度：

```cpp
string<10> str;
str.insert(0, "Hello");      // ✅ 编译通过 (5 <= 10)
str.insert(0, "This is a very long string");  // ❌ 编译错误
```

### 运行时检查
当使用 `const char*` 指针时，只能在运行时检查：

```cpp
string<10> str;
const char* dynamicStr = "Some text";
str.insert(0, dynamicStr);   // ✅ 运行时检查
```

如果容量不足，操作会失败并记录错误日志。

## 日志输出示例

当容量溢出时，会输出详细的错误日志：

```
[2025-10-19 17:01:36.404] [ERROR] [string.inl:129] String insert failed: capacity overflow! 
Current size: 10, trying to insert: 1, capacity: 10
```

## 性能特点

- **零堆分配**：所有数据在栈上，无动态内存分配
- **内联函数**：大部分函数都是内联的，性能优异
- **编译时优化**：字符串字面量在编译时检查
- **无异常**：所有函数标记为 `noexcept`

## 依赖

- **zerocp_foundationLib/vocabulary** - 字符串类实现
- **zerocp_foundationLib/report** - 日志系统
- **C++17 或更高版本**
- **线程库**（pthread）- 用于日志后端

## 注意事项

1. **容量限制**：字符串容量在编译时固定，无法动态增长
2. **溢出处理**：容量溢出时，插入操作会失败但不会崩溃
3. **日志异步**：日志输出是异步的，需要等待一小段时间才能看到输出
4. **栈空间**：注意不要创建过大容量的字符串，避免栈溢出

## 测试结果

所有测试均通过 ✅

- 基本功能测试：通过
- 拷贝/移动语义：通过
- 容量检查：通过
- 日志集成：通过
- 栈分配验证：通过

