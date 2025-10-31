// 版权所有 (c) 2019 Robert Bosch GmbH 保留所有权利。
// 版权所有 (c) 2021 - 2023 Apex.AI Inc. 保留所有权利。
//
// 根据 Apache 许可证 2.0 版（“许可证”）获得许可；
// 除非遵守许可证，否则您不得使用此文件。
// 您可以在以下网址获取许可证副本：
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// 除非适用法律要求或书面同意，软件
// 根据许可证分发的按“原样”分发，
// 无任何明示或暗示的担保或条件。
// 请参阅许可证以了解管理权限和
// 许可证下的限制。
//
// SPDX-License-Identifier: Apache-2.0
#ifndef ZEROCP_VOCABULARY_VECTOR_HPP
#define ZEROCP_VOCABULARY_VECTOR_HPP

#include <cstdint>
#include <array>
#include <type_traits>
#include "macros.hpp"
#include "algorithm.hpp"
#include "logging.hpp"

// 简化的 UninitializedArray 实现
template<typename T, uint64_t Capacity>
using UninitializedArray = std::array<std::aligned_storage_t<sizeof(T), alignof(T)>, Capacity>;

namespace ZeroCP
{
/// @brief  C++11 兼容的 vector 实现。由于我们不使用异常并且需要一个可以完全位于共享内存中的数据结构，因此我们对 API 进行了一些调整。
///
/// @attention 越界访问或访问空向量可能导致程序终止！
///
template <typename T, uint64_t Capacity>
class vector final
{
  public:
    using value_type = T; // 元素类型
    using iterator = T*; // 迭代器类型
    using reference = T&; // 引用类型
    using const_iterator = const T*; // 常量迭代器类型
    using const_reference = const T&; // 常量引用类型
    using difference_type = std::ptrdiff_t; // 差值类型
    using size_type = decltype(Capacity); // 大小类型
    using index_type = size_type; // 索引类型

    /// @brief 创建一个空向量
    vector() noexcept = default;

    /// @brief 创建一个包含 count 个元素的向量，每个元素的值为 value
    /// @param [in] count 要插入向量的副本数量
    /// @param [in] value 要插入向量的值
    vector(const uint64_t count, const T& value) noexcept;

    /// @brief 创建一个包含 count 个元素的向量，每个元素使用 T 的默认构造函数构造
    /// @param [in] count 要插入向量的副本数量
    explicit vector(const uint64_t count) noexcept;

    /// @brief 拷贝构造函数，用于拷贝相同容量的向量
    /// @param[in] rhs 拷贝源
    vector(const vector& rhs) noexcept;

    /// @brief 移动构造函数，用于移动相同容量的向量
    /// @param[in] rhs 移动源
    vector(vector&& rhs) noexcept;

    /// @brief 析构向量，并以相反的构造顺序调用所有包含元素的析构函数
    ~vector() noexcept;

    /// @brief 拷贝赋值。如果目标向量包含的元素多于源，则剩余元素将被析构
    /// @param[in] rhs 拷贝源
    /// @return 自身的引用
    vector& operator=(const vector& rhs) noexcept;

    /// @brief 移动赋值。如果目标向量包含的元素多于源，则剩余元素将被析构
    /// @param[in] rhs 移动源
    /// @return 自身的引用
    vector& operator=(vector&& rhs) noexcept;

    /// @brief 返回指向向量第一个元素的迭代器，如果向量为空，则返回与 end 相同的迭代器（第一个在向量之外的迭代器）
    iterator begin() noexcept;

    /// @brief 返回指向向量第一个元素的常量迭代器，如果向量为空，则返回与 end 相同的迭代器（第一个在向量之外的迭代器）
    const_iterator begin() const noexcept;

    /// @brief 返回指向最后一个元素之后的元素的迭代器（第一个在向量之外的元素）
    iterator end() noexcept;

    /// @brief 返回指向最后一个元素之后的元素的常量迭代器（第一个在向量之外的元素）
    const_iterator end() const noexcept;

    /// @brief 返回指向底层数组的指针
    /// @return 指向底层数组的指针
    T* data() noexcept;

    /// @brief 返回指向底层数组的常量指针
    /// @return 指向底层数组的常量指针
    const T* data() const noexcept;

    /// @brief 返回存储在索引处的元素的引用。
    /// @param[in] index 要返回的元素的索引
    /// @return 存储在索引处的元素的引用
    /// @attention 越界访问会导致程序终止！
    T& at(const uint64_t index) noexcept;

    /// @brief 返回存储在索引处的元素的常量引用。
    /// @param[in] index 要返回的元素的索引
    /// @return 存储在索引处的元素的常量引用
    /// @attention 越界访问会导致程序终止！
    const T& at(const uint64_t index) const noexcept;

    /// @brief 返回存储在索引处的元素的引用。
    /// @param[in] index 要返回的元素的索引
    /// @return 存储在索引处的元素的引用
    /// @attention 越界访问会导致程序终止！
    T& operator[](const uint64_t index) noexcept;

    /// @brief 返回存储在索引处的元素的常量引用。
    /// @param[in] index 要返回的元素的索引
    /// @return 存储在索引处的元素的常量引用
    /// @attention 越界访问会导致程序终止！
    const T& operator[](const uint64_t index) const noexcept;

    /// @brief 返回第一个元素的引用；如果向量为空则终止
    /// @return 第一个元素的引用
    /// @attention 访问空向量会导致程序终止！
    T& front() noexcept;

    /// @brief 返回第一个元素的常量引用；如果向量为空则终止
    /// @return 第一个元素的常量引用
    /// @attention 访问空向量会导致程序终止！
    const T& front() const noexcept;

    /// @brief 返回最后一个元素的引用；如果向量为空则终止
    /// @return 最后一个元素的引用
    /// @attention 访问空向量会导致程序终止！
    T& back() noexcept;

    /// @brief 返回最后一个元素的常量引用；如果向量为空则终止
    /// @return 最后一个元素的常量引用
    /// @attention 访问空向量会导致程序终止！
    const T& back() const noexcept;

    /// @brief 返回通过模板参数给定的向量容量
    static constexpr uint64_t capacity() noexcept;

    /// @brief 返回当前存储在向量中的元素数量
    uint64_t size() const noexcept;

    /// @brief 如果向量为空则返回 true，否则返回 false
    bool empty() const noexcept;

    /// @brief 调用所有包含元素的析构函数并移除它们
    void clear() noexcept;

    /// @brief 调整向量大小。如果向量大小增加，将使用给定参数构造新元素。如果 count 大于容量，向量将保持不变。如果 count 小于大小，剩余元素将被移除，不会构造新元素。
    /// @param[in] count 向量的新大小
    /// @param[in] args 用于构造新创建元素的参数
    /// @return 如果调整大小成功则返回 true，如果 count 大于容量则返回 false。
    /// @note 这里显式不需要完美转发参数。想想如果 resize 通过移动构造创建两个新元素会发生什么。第一个有一个有效的源，但第二个得到一个已经移动的参数。
    template <typename... Targs>
    bool resize(const uint64_t count, const Targs&... args) noexcept;

    /// @brief 将所有参数转发给包含元素的构造函数，并在提供的位置执行 placement new
    /// @param[in] position 应创建元素的位置
    /// @param[in] args 用于构造新创建参数的参数
    /// @return 如果成功则返回 true，如果位置大于大小或向量已满则返回 false
    template <typename... Targs>
    bool emplace(const uint64_t position, Targs&&... args) noexcept;

    /// @brief 将所有参数转发给包含元素的构造函数，并在末尾执行 placement new
    /// @param[in] args 用于构造新创建参数的参数
    /// @return 如果成功则返回 true，如果向量已满则返回 false
    template <typename... Targs>
    bool emplace_back(Targs&&... args) noexcept;

    /// @brief 将给定元素附加到向量的末尾
    /// @param[in] value 要附加到向量的值
    /// @return 如果成功则返回 true，如果向量已满则返回 false
    bool push_back(const T& value) noexcept;

    /// @brief 将给定元素附加到向量的末尾
    /// @param[in] value 要附加到向量的值
    /// @return 如果成功则返回 true，如果向量已满则返回 false
    bool push_back(T&& value) noexcept;

    /// @brief 移除向量的最后一个元素；在空容器上调用 pop_back 不执行任何操作
    /// @return 如果最后一个元素被移除则返回 true。如果向量为空则返回 false。
    bool pop_back() noexcept;

    /// @brief 移除给定位置的元素。如果该元素位于向量中间，则每个元素都向左移动一个位置，以确保元素连续存储
    /// @param[in] position 要移除元素的位置
    /// @return 如果元素被移除则返回 true，即 begin() <= position < end()，否则返回 false
    bool erase(iterator position) noexcept;

  private:
    T& at_unchecked(const uint64_t index) noexcept;
    const T& at_unchecked(const uint64_t index) const noexcept;

    void clearFrom(const uint64_t startPosition) noexcept;

    // AXIVION Next Construct AutosarC++19_03-A1.1.1 : 对象大小取决于模板参数，必须在特定的模板实例化中处理
    UninitializedArray<T, Capacity> m_data{};
    uint64_t m_size{0U};
};

// AXIVION Next Construct AutosarC++19_03-A13.5.5 : 有意实现不同参数以启用不同容量的向量比较
template <typename T, uint64_t CapacityLeft, uint64_t CapacityRight>
constexpr bool operator==(const vector<T, CapacityLeft>& lhs, const vector<T, CapacityRight>& rhs) noexcept;

// AXIVION Next Construct AutosarC++19_03-A13.5.5 : 有意实现不同参数以启用不同容量的向量比较
template <typename T, uint64_t CapacityLeft, uint64_t CapacityRight>
constexpr bool operator!=(const vector<T, CapacityLeft>& lhs, const vector<T, CapacityRight>& rhs) noexcept;
} // namespace iox

#include "../detail/vector.inl"

#endif // IOX_HOOFS_CONTAINER_VECTOR_HPP
