#ifndef ZeroCP_Builder_HPP
#define ZeroCP_Builder_HPP

/**
 * @brief ZeroCP_Builder_Implementation 宏用于定义Builder模式常用的链式设置函数和成员变量。
 *
 * 用法：在类定义内部使用此宏可快速生成 builder 风格的 setter 方法和对应的成员变量。
 * - 自动生成两个重载 setter（左值/右值引用），便于链式构造。
 * - 默认成员变量初始化为 defaultvalue。
 *
 * @param type          成员变量类型
 * @param name          setter 函数名，同时形成成员变量 m_name
 * @param defaultvalue  成员变量默认值
 *
 * 示例:
 *   class MyClass {
 *       ZeroCP_Builder_Implementation(int, age, 0)
 *   };
 *
 *   可链式用法：
 *     MyClass().age(10).xxx(...);
 */
#define ZeroCP_Builder_Implementation(type, name, defaultvalue)                    \
public:                                                                            \
    /**                                                                            \
     * @brief builder setter —— 左值引用方式                                         \
     * @param value 赋值内容                                                        \
     * @return 当前对象右值引用                                                      \
     */                                                                            \
    decltype(auto) name(type& value) && noexcept                                   \
    {                                                                              \
        m_##name = value;                                                          \
        return std::move(*this);                                                   \
    }                                                                              \
    /**                                                                            \
     * @brief builder setter —— 右值引用方式                                         \
     * @param value 赋值内容                                                        \
     * @return 当前对象右值引用                                                      \
     */                                                                            \
    decltype(auto) name(type&& value) && noexcept                                  \
    {                                                                              \
        m_##name = std::move(value);                                               \
        return std::move(*this);                                                   \
    }                                                                              \
                                                                                   \
private:                                                                           \
    type m_##name{defaultvalue};

#endif