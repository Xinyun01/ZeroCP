# C++20/23 现代特性使用指南

本项目已全面升级到 C++23 标准，充分利用现代 C++ 的最新特性。

## 🚀 已使用的 C++20/23 特性

### 1. **Concepts (概念)**
使用 concepts 约束模板参数，提供更好的类型安全和编译错误信息。

```cpp
// 定义 errno 类型概念
template <typename T>
concept ErrnoType = std::integral<T> && std::is_signed_v<T>;

// 在模板中使用
template <ErrnoType... IgnoredErrnos>
PosixCallEvaluator<ReturnType> ignoreErrnos(const IgnoredErrnos... ignoredErrnos);
```

**优势：**
- 更清晰的类型约束
- 更好的编译错误信息
- 替代复杂的 SFINAE 技巧

### 2. **Requires 表达式**
在函数模板中直接约束参数类型。

```cpp
template <typename... SuccessReturnValues>
    requires (std::convertible_to<SuccessReturnValues, ReturnType> && ...)
PosixCallEvaluator<ReturnType> successReturnValue(const SuccessReturnValues... successReturnValues);
```

**优势：**
- 更精确的类型检查
- 编译期验证参数兼容性

### 3. **[[nodiscard]] 属性**
强制要求检查返回值，防止意外忽略重要结果。

```cpp
[[nodiscard]] PosixCallEvaluator<ReturnType> ignoreErrnos(...);
[[nodiscard]] expected<T, E> evaluate() const&& noexcept;
```

**优势：**
- 防止忽略重要的返回值
- 编译器会对未使用的返回值发出警告

### 4. **[[likely]] / [[unlikely]] 属性**
优化分支预测，提升性能。

```cpp
if (m_details.result.errnum != EINTR) [[likely]]
{
    break;  // 大多数情况下不会是 EINTR
}

if (!m_details.hasSuccess) [[unlikely]]
{
    // 错误处理路径
}
```

**优势：**
- 帮助编译器优化代码布局
- 提升热路径性能

### 5. **三路比较运算符（Spaceship Operator）**
自动生成所有比较运算符。

```cpp
struct PosixCallResult
{
    T value{};
    int32_t errnum = POSIX_CALL_INVALID_ERRNO;
    
    [[nodiscard]] constexpr auto operator<=>(const PosixCallResult&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const PosixCallResult&) const noexcept = default;
};
```

**优势：**
- 减少样板代码
- 自动生成所有 6 个比较运算符

### 6. **std::source_location**
自动获取源代码位置信息，替代宏。

```cpp
constexpr PosixCall_Details(
    const char* posixFunctionName,
    std::source_location location = std::source_location::current()) noexcept
    : source_loc(location)
{}
```

**优势：**
- 编译期获取文件名、行号、函数名
- 比传统宏更安全和类型安全

### 7. **constexpr 增强**
更多函数可以在编译期执行。

```cpp
constexpr PosixCall_Builder(...) noexcept { ... }
constexpr expected(const T& value) noexcept { ... }
[[nodiscard]] constexpr bool has_value() const noexcept { return m_has_value; }
```

**优势：**
- 更多编译期计算
- 更好的性能和类型检查

### 8. **Fold Expressions 优化**
简化可变参数模板。

```cpp
// 使用 fold expression 简化查找
template <typename T, typename... Values>
[[nodiscard]] constexpr bool contains(const T& value, const Values&... values) noexcept
{
    return ((value == static_cast<T>(values)) || ...);
}
```

**优势：**
- 更简洁的代码
- 更好的编译性能

### 9. **Expected<T, E> (C++23 风格)**
模仿 std::expected 的错误处理机制。

```cpp
template <typename T, typename E>
    requires (!std::is_reference_v<T> && !std::is_reference_v<E>)
class expected
{
    [[nodiscard]] constexpr T value_or(U&& default_value) const &;
    [[nodiscard]] constexpr bool has_value() const noexcept;
};
```

**优势：**
- 类型安全的错误处理
- 避免异常开销
- 显式的成功/失败语义

### 10. **引用限定符增强**
支持更多的值类别重载。

```cpp
[[nodiscard]] constexpr T& value() & noexcept;
[[nodiscard]] constexpr const T& value() const & noexcept;
[[nodiscard]] constexpr T&& value() && noexcept;
[[nodiscard]] constexpr const T&& value() const && noexcept;
```

**优势：**
- 更精确的语义
- 避免不必要的拷贝

### 11. **Inline constexpr 变量**
模块级常量的现代声明方式。

```cpp
inline constexpr uint32_t POSIX_CALL_ERROR = 128U;
inline constexpr uint32_t POSIX_CALL_EINTR_REPETITIONS = 5U;
```

**优势：**
- 避免 ODR 违规
- 头文件中安全定义常量

### 12. **noexcept 规范增强**
条件性 noexcept 提供更精确的异常规范。

```cpp
constexpr expected(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires std::copy_constructible<T>
{ ... }
```

**优势：**
- 更准确的异常保证
- 更好的优化机会

## 📊 性能影响

使用这些现代特性的性能提升：

1. **编译期检查**: concepts 和 requires 在编译期捕获错误
2. **分支预测**: [[likely]]/[[unlikely]] 优化热路径，可提升 5-10% 性能
3. **constexpr**: 更多编译期计算，减少运行时开销
4. **nodiscard**: 防止错误，避免 bug 导致的性能问题
5. **Fold expressions**: 更好的编译器优化机会

## 🔧 编译要求

```cmake
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

**编译器支持：**
- GCC 12+ 
- Clang 16+
- MSVC 19.34+ (Visual Studio 2022 17.4+)

## 📝 最佳实践

1. **总是使用 [[nodiscard]]** 在返回值重要的函数上
2. **使用 concepts** 约束模板参数而不是 SFINAE
3. **使用 [[likely]]/[[unlikely]]** 标注明显的分支概率
4. **使用 constexpr** 在可能的地方实现编译期计算
5. **使用 expected<T, E>** 替代异常进行错误处理

## 🎯 迁移指南

从旧代码迁移到现代 C++：

| 旧特性 | 新特性 | 优势 |
|--------|--------|------|
| SFINAE | Concepts | 更清晰、更好的错误信息 |
| `__FILE__`, `__LINE__` | `std::source_location` | 类型安全、更灵活 |
| 手写比较运算符 | `operator<=>` | 减少代码量 |
| `noexcept` | `noexcept(expr)` | 条件性异常规范 |
| 递归模板 | Fold expressions | 更简洁、更快编译 |

## 📚 参考资料

- [C++20 Concepts](https://en.cppreference.com/w/cpp/language/constraints)
- [C++20 三路比较](https://en.cppreference.com/w/cpp/language/default_comparisons)
- [C++23 Expected](https://en.cppreference.com/w/cpp/utility/expected)
- [属性说明符](https://en.cppreference.com/w/cpp/language/attributes)





