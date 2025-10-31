// C++20/23 header-only algorithms used across foundationLib
#ifndef ZEROCP_FOUNDATIONLIB_STATICSTL_ALGORITHM_HPP
#define ZEROCP_FOUNDATIONLIB_STATICSTL_ALGORITHM_HPP

#include <concepts>
#include <type_traits>

namespace ZeroCP
{
namespace algorithm
{

// C++20: 使用 concepts 约束模板参数
template <typename T>
    requires std::equality_comparable<T>
[[nodiscard]] constexpr bool doesContainValue(const T&) noexcept
{
    return false;
}

// C++20: 使用 fold expressions 和 concepts 优化
template <typename T, typename First, typename... Rest>
    requires std::equality_comparable<T> && 
             std::convertible_to<First, T> && 
             (std::convertible_to<Rest, T> && ...)
[[nodiscard]] constexpr bool doesContainValue(const T& value, const First& first, const Rest&... rest) noexcept
{
    // C++20: 使用更高效的实现
    if constexpr (sizeof...(Rest) == 0)
    {
        return value == static_cast<T>(first);
    }
    else
    {
        return (value == static_cast<T>(first)) || doesContainValue<T>(value, rest...);
    }
}

// C++20: 使用 fold expression 的替代实现（更简洁）
template <typename T, typename... Values>
    requires std::equality_comparable<T> && (std::convertible_to<Values, T> && ...)
[[nodiscard]] constexpr bool contains(const T& value, const Values&... values) noexcept
{
    return ((value == static_cast<T>(values)) || ...);
}

// 最小值函数
template <typename T>
    requires std::totally_ordered<T>
[[nodiscard]] constexpr const T& minVal(const T& a, const T& b) noexcept
{
    return (a < b) ? a : b;
}

// 最大值函数
template <typename T>
    requires std::totally_ordered<T>
[[nodiscard]] constexpr const T& maxVal(const T& a, const T& b) noexcept
{
    return (a > b) ? a : b;
}

} // namespace algorithm
} // namespace ZeroCP

#endif // ZEROCP_FOUNDATIONLIB_STATICSTL_ALGORITHM_HPP


