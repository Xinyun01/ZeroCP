#ifndef POSIX_CALL_HPP
#define POSIX_CALL_HPP

#include <cstdint>
#include <cerrno>
#include <concepts>
#include <source_location>
#include <expected>  // C++23: std::expected
#include "../../../staticstl/include/algorithm.hpp"

namespace ZeroCp 
{

// 使用 inline constexpr 变量 (C++17+)
inline constexpr uint32_t POSIX_CALL_ERROR = 128U;  // 错误消息缓冲区大小
inline constexpr uint32_t POSIX_CALL_EINTR_REPETITIONS = 5U;  // EINTR 最大重试次数
inline constexpr int32_t POSIX_CALL_INVALID_ERRNO = -1;  // 无效 errno 标记值

// C++20 Concept: 限制可以作为 errno 值的类型
template <typename T>
concept ErrnoType = std::integral<T> && std::is_signed_v<T>;

// C++20 结构体，使用默认成员初始化
template <typename T>
struct PosixCallResult
{
    T value{};
    int32_t errnum = POSIX_CALL_INVALID_ERRNO;
    
    // C++20: 三路比较运算符（spaceship operator）
    constexpr auto operator<=>(const PosixCallResult&) const noexcept = default;
    constexpr bool operator==(const PosixCallResult&) const noexcept = default;
};


// 前置声明
template <typename ReturnType, typename... FunctionArguments>
class PosixCall_Builder;

namespace Details
{
    // C++20: 使用 designated initializers 和更现代的结构设计
    template <typename ReturnType>
    struct PosixCall_Details
    {
        // C++20: 使用 std::source_location 获取调用位置（C++20）
        constexpr PosixCall_Details(
            const char* posixFunctionName, 
            const char* file, 
            int line, 
            const char* callingFunction,
            std::source_location location = std::source_location::current()) noexcept
            : posixFunctionName(posixFunctionName)
            , file(file)
            , callingFunction(callingFunction)
            , line(line)
            , source_loc(location)
        {}
        
        const char* posixFunctionName = nullptr;
        const char* file = nullptr;
        const char* callingFunction = nullptr;
        int32_t line = 0;
        std::source_location source_loc{};  // C++20: 源位置信息
        bool hasSuccess = true;
        bool hasIgnoredErrno = false;
        bool hasSilentErrno = false;
        PosixCallResult<ReturnType> result;
    };
    
    // 类作为函数的返回类型
    template <typename ReturnType, typename... FunctionArguments>
    PosixCall_Builder<ReturnType, FunctionArguments...> CreatePosixCall_Builder(
        ReturnType (*ZeroCp_PosixCall)(FunctionArguments...),
        const char* posixFunctionName,
        const char* file,
        const int32_t line,
        const char* callingFunction) noexcept;
}

// 前置声明
template <typename ReturnType>
class PosixCallEvaluator;

// ==================== PosixCall_Evaluator ====================
// 评估器：提供访问调用结果的接口
template <typename ReturnType>
class PosixCallEvaluator
{
    public:
    template <typename... IgnoredErrnos>
    PosixCallEvaluator<ReturnType> ignoreErrnos(const IgnoredErrnos... ignoredErrnos) const&& noexcept;

    /// @brief 在评估中静默指定的 errno，不打印错误消息
    /// @tparam SilentErrnos 一组 int32_t 变量
    /// @param[in] silentErrnos 应静默且不导致错误日志的 errno 的 int32_t 值
    /// @return 用于进一步设置评估的 PosixCallEvaluator
    template <typename... SilentErrnos>
    PosixCallEvaluator<ReturnType> suppressErrorMessagesForErrnos(const SilentErrnos... silentErrnos) const&& noexcept;

    /// @brief 评估 POSIX 调用的结果
    /// @return 返回一个 std::expected，其中在两种情况下都包含一个 PosixCallResult<ReturnType>，其中包含函数调用的返回值
    /// (.value) 和 errno 值 (.errnum)
    /// 成功时返回 expected 的 value，失败时返回 unexpected 中的 error
    std::expected<PosixCallResult<ReturnType>, PosixCallResult<ReturnType>> evaluate() const&& noexcept;
    private:
    template <typename ReturnTypeFriend>
    friend class PosixCallVerificator;

    explicit PosixCallEvaluator(Details::PosixCall_Details<ReturnType>& details) noexcept;
    Details::PosixCall_Details<ReturnType>& m_details;
};

// ==================== PosixCallVerificator ====================
// 负责验证系统调用的返回值，支持可变参数的成功/失败值列表
template <typename ReturnType>
class PosixCallVerificator
{
    public:
    /// @brief POSIX 函数调用通过单个值定义成功
    /// @param[in] successReturnValues 定义成功的一组值
    /// @return 评估 errno 值的 PosixCallEvaluator
    template <typename... SuccessReturnValues>
    PosixCallEvaluator<ReturnType> successReturnValue(const SuccessReturnValues... successReturnValues) && noexcept;

    /// @brief POSIX 函数调用通过单个值定义失败
    /// @param[in] failureReturnValues 定义失败的一组值
    /// @return 评估 errno 值的 PosixCallEvaluator
    template <typename... FailureReturnValues>
    PosixCallEvaluator<ReturnType> failureReturnValue(const FailureReturnValues... failureReturnValues) && noexcept;

    /// @brief POSIX 函数调用通过返回 errno 值而不是设置 errno 来定义失败
    /// @return 评估 errno 值的 PosixCallEvaluator
    PosixCallEvaluator<ReturnType> returnValueMatchesErrno() && noexcept;
    private:
        template <typename ReturnTypeFriend, typename... FunctionArgumentsFriend>
        friend class PosixCall_Builder;

        explicit PosixCallVerificator(Details::PosixCall_Details<ReturnType>& details) noexcept;
        Details::PosixCall_Details<ReturnType>& m_details;
};

// ==================== PosixCall_Builder ====================
// 系统构建器：执行 POSIX 调用并自动处理 EINTR，返回 Verifier 进行验证
// C++20: 使用 concepts 约束函数类型
template <typename ReturnType, typename... FunctionArguments>
class PosixCall_Builder
{
    public:
        /// @brief 输入函数类型
        using FunctionType_t = ReturnType (*)(FunctionArguments...);

        PosixCallVerificator<ReturnType> operator()(FunctionArguments... arguments) && noexcept;

    private:
    template <typename ReturnTypeFriend, typename... FunctionArgumentsFriend>
    friend PosixCall_Builder<ReturnTypeFriend, FunctionArgumentsFriend...>
    Details::CreatePosixCall_Builder(ReturnTypeFriend (*ZeroCp_PosixCall)(FunctionArgumentsFriend...),
                                   const char* posixFunctionName,
                                   const char* file,
                                   const int32_t line,
                                   const char* callingFunction) noexcept;

    PosixCall_Builder(FunctionType_t ZeroCp_PosixCall,
                     const char* posixFunctionName,
                     const char* file,
                     const int32_t line,
                     const char* callingFunction) noexcept;
        FunctionType_t m_ZeroCp_PosixCall;
        Details::PosixCall_Details<ReturnType> m_details;
};

}  // namespace ZeroCp


#include "detial/posix_call.inl"

// 调用构造去使用builder 去启用
#define ZeroCp_PosixCall(function) \
    ZeroCp::Details::CreatePosixCall_Builder(&function, \
                                            (#function), \
                                            __FILE__, \
                                            __LINE__, \
                                            __PRETTY_FUNCTION__)

#endif  // POSIX_CALL_HPP
