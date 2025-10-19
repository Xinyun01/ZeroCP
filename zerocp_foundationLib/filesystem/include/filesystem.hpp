#pragma once

#include <cstdint>
#include <string>
#include <type_traits>

namespace ZeroCP
{

// 文件访问模式
enum class AccessMode : uint8_t
{
    ReadOnly = 0,    // 只读
    WriteOnly = 1,   // 只写
    ReadWrite = 2    // 读写
};

// 共享内存打开模式
enum class OpenMode : uint8_t
{
    /// @brief 创建共享内存，如果已存在则构造失败
    ExclusiveCreate = 0U,
    /// @brief 创建共享内存，如果已存在则删除并重新创建
    PurgeAndCreate = 1U,
    /// @brief 创建共享内存，如果不存在则创建，否则打开已存在的
    OpenOrCreate = 2U,
    /// @brief 打开已存在的共享内存，如果不存在则失败
    OpenExisting = 3U
};

// 权限枚举，表示简单读写权限
enum class Permissions : uint8_t
{
    None = 0,      // 无权限
    Read = 1,      // 只读
    Write = 2,     // 只写
    ReadWrite = 3  // 读写
};

// POSIX风格的详细权限枚举
enum class Perms : uint16_t {
    None        = 0,    // 无权限
    OwnerRead   = 0400, // 所有者可读
    OwnerWrite  = 0200, // 所有者可写
    OwnerExec   = 0100, // 所有者可执行
    OwnerAll    = 0700, // 所有者全部权限
    GroupRead   = 040,  // 组可读
    GroupWrite  = 020,  // 组可写
    GroupExec   = 010,  // 组可执行
    GroupAll    = 070,  // 组全部权限
    OthersRead  = 04,   // 其他用户可读
    OthersWrite = 02,   // 其他用户可写
    OthersExec  = 01,   // 其他用户可执行
    OthersAll   = 07,   // 其他用户全部权限
    All         = 0777  // 全部权限
};

// 位运算符重载（使 enum class 支持位运算）
constexpr Perms operator|(Perms a, Perms b) noexcept {
    using T = std::underlying_type_t<Perms>;
    return static_cast<Perms>(static_cast<T>(a) | static_cast<T>(b));
}

constexpr Perms operator&(Perms a, Perms b) noexcept {
    using T = std::underlying_type_t<Perms>;
    return static_cast<Perms>(static_cast<T>(a) & static_cast<T>(b));
}

// 提取底层值，常用于与POSIX接口交互
constexpr uint16_t to_mode(Perms p) noexcept {
    return static_cast<uint16_t>(p);
}

// 添加前导斜杠（如果名称不为空且不以斜杠开头）
inline std::string addLeadingSlash(const std::string& name) noexcept
{
    if (!name.empty() && name[0] != '/')
    {
        return "/" + name;
    }
    return name;
}

}

