# 代码风格指南

## 行宽限制

本项目设置 **最大行宽为 120 个字符**。

## 配置文件说明

### 1. `.vscode/settings.json` - Cursor/VS Code 编辑器配置
- 显示 120 字符标尺线
- 设置 C/C++ 文件的缩进和格式

**效果**: 在编辑器中会看到一条垂直的标尺线，提示你不要超过 120 字符。

### 2. `.editorconfig` - 跨编辑器配置
- 支持多种编辑器（VS Code, Vim, Emacs, IntelliJ 等）
- 统一团队代码风格
- 设置行宽、缩进、换行符等

**使用**: 大多数现代编辑器会自动识别并应用此配置。

### 3. `.clang-format` - 自动代码格式化
- 基于 LLVM 风格
- 行宽限制 120 字符
- 自动格式化代码

**使用方法**:

```bash
# 格式化单个文件
clang-format -i your_file.cpp

# 格式化整个目录
find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# 在 Cursor 中使用快捷键
# Linux: Ctrl+Shift+I
# Mac: Cmd+Shift+I
```

### 4. `CPPLINT.cfg` - 代码检查配置
- Google C++ 代码风格检查工具配置
- 行宽设置为 120

**使用方法**:

```bash
# 安装 cpplint
pip install cpplint

# 检查文件
cpplint your_file.cpp

# 检查整个项目
cpplint --recursive .
```

## 在 Cursor 中启用

### 方法 1: 查看标尺线
1. 重启 Cursor 或重新加载窗口
2. 打开任意 C++ 文件
3. 你会看到第 120 列有一条垂直线

### 方法 2: 使用 Clang-Format
1. 安装 C/C++ 扩展（通常已安装）
2. 在文件中右键 → "格式化文档"
3. 或使用快捷键 `Ctrl+Shift+I`

### 方法 3: 保存时自动格式化
在 `.vscode/settings.json` 中添加：
```json
{
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd"
}
```

## 手动换行建议

当一行代码超过 120 字符时，建议的换行位置：

### 1. 函数声明
```cpp
// ❌ 太长
template <typename ReturnType, typename... Arguments>
PosixCall_Builder<ReturnType, Arguments...> CreatePosixCall_Builder(ReturnType (*ZeroCp_PosixCall)(Arguments...), const char* function_name, const char* file_name, int line_number, const char* pretty_function_name)

// ✅ 推荐
template <typename ReturnType, typename... Arguments>
PosixCall_Builder<ReturnType, Arguments...> CreatePosixCall_Builder(
    ReturnType (*ZeroCp_PosixCall)(Arguments...),
    const char* function_name,
    const char* file_name,
    int line_number,
    const char* pretty_function_name
)
```

### 2. 宏定义
```cpp
// ❌ 太长
#define ZeroCp_PosixCall(function) ZeroCp::detail::CreatePosixCall_Builder(&function, (#function), __FILE__, __LINE__, __PRETTY_FUNCTION__)

// ✅ 推荐
#define ZeroCp_PosixCall(function)                                  \
    ZeroCp::detail::CreatePosixCall_Builder(                        \
        &function,                                                  \
        (#function),                                                \
        __FILE__,                                                   \
        __LINE__,                                                   \
        __PRETTY_FUNCTION__                                         \
    )
```

### 3. 函数调用
```cpp
// ❌ 太长
auto result = some_very_long_function_name(argument1, argument2, argument3, argument4, argument5);

// ✅ 推荐
auto result = some_very_long_function_name(
    argument1,
    argument2,
    argument3,
    argument4,
    argument5
);
```

### 4. 条件语句
```cpp
// ❌ 太长
if (very_long_condition_1 && very_long_condition_2 && very_long_condition_3 && very_long_condition_4) {

// ✅ 推荐
if (very_long_condition_1 && 
    very_long_condition_2 && 
    very_long_condition_3 && 
    very_long_condition_4) {
```

## 检查现有代码

```bash
# 查找超过 120 字符的行
find . -name "*.cpp" -o -name "*.hpp" | xargs awk 'length > 120 {print FILENAME":"NR":"length":"$0}'

# 使用 grep 查找（更简单）
grep -rn "^.\{121,\}$" --include="*.cpp" --include="*.hpp" .
```

## 工具安装

```bash
# 安装 clang-format
sudo apt-get install clang-format

# 安装 cpplint
pip install cpplint

# 验证安装
clang-format --version
cpplint --version
```

## 团队协作

建议将以下文件提交到版本控制：
- ✅ `.editorconfig`
- ✅ `.clang-format`
- ✅ `CPPLINT.cfg`
- ✅ `CODE_STYLE.md`
- ❌ `.vscode/settings.json` (可选，看团队偏好)

这样团队成员都能使用统一的代码风格。
