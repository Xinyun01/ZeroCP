#ifndef POSIX_CALL_HPP
#define POSIX_CALL_HPP

#include <cstdint>
#include <cerrno>

namespace ZeroCp 
{

static constexpr uint32_t POSIX_CALL_ERROR = 128U;  // 错误消息缓冲区大小
static constexpr uint32_t POSIX_CALL_EINTR_REPETITIONS = 5U;  // EINTR 最大重试次数
static constexpr int32_t POSIX_CALL_INVALID_ERRNO = -1;  // 无效 errno 标记值

template <typename T>
struct PosixCall_Result
{
    T value{};
    int32_t errnum = POSIX_CALL_INVALID_ERRNO;
};

namespace algorithm
{
    // 可变参数模板：判断 value 是否在 values... 中
    template <typename T, typename... Values>
    inline constexpr bool doesContainValue(const T& value, const Values&... values) noexcept
    {
        return ((value == values) || ...);  // C++17 折叠表达式
    }
}

// 前置声明
template <typename ReturnType, typename... Arguments>
class PosixCall_Builder;

namespace detail
{
    // 用于接受PosixCall_Builder 中的 function_name, file_name, line_number, pretty_function_name
    template <typename ReturnType>
    struct PosixCall_Details
    {
        const char* function_name;
        const char* file_name;
        int line_number;
        const char* pretty_function_name;
        PosixCall_Result<ReturnType> result;
        bool hasSuccess{false};  // 验证结果标志
    };
    
    // 类作为函数的返回类型
    template <typename ReturnType, typename... Arguments>
    PosixCall_Builder<ReturnType, Arguments...> CreatePosixCall_Builder(
        ReturnType (*ZeroCp_PosixCall)(Arguments...),
        const char* function_name,
        const char* file_name,
        int line_number,
        const char* pretty_function_name) noexcept
    {
        return PosixCall_Builder<ReturnType, Arguments...>(
            ZeroCp_PosixCall, function_name, file_name, line_number, pretty_function_name);
    }
}

// 前置声明
template <typename ReturnType>
class PosixCall_Evaluator;

// ==================== PosixCall_Verifier ====================
// 负责验证系统调用的返回值，支持可变参数的成功/失败值列表
template <typename ReturnType>
class PosixCall_Verifier
{
public:
    // 构造函数：接收调用详情
    PosixCall_Verifier(detail::PosixCall_Details<ReturnType>& details) noexcept
        : m_details{details}
    {
    }
    
    // 指定成功返回值列表（可变参数）
    // 示例：.successReturnValue(0) 或 .successReturnValue(0, 1, 2)
    template <typename... SuccessReturnValues>
    inline PosixCall_Evaluator<ReturnType>
    successReturnValue(const SuccessReturnValues... successReturnValues) && noexcept
    {
        m_details.hasSuccess = algorithm::doesContainValue(m_details.result.value, successReturnValues...);
        return PosixCall_Evaluator<ReturnType>(m_details);
    }
    
    // 指定失败返回值列表（可变参数）
    // 示例：.failureReturnValue(-1) 或 .failureReturnValue(-1, 0)
    template <typename... FailureReturnValues>
    inline PosixCall_Evaluator<ReturnType>
    failureReturnValue(const FailureReturnValues... failureReturnValues) && noexcept
    {
        using ValueType = decltype(m_details.result.value);
        m_details.hasSuccess = 
            !algorithm::doesContainValue(m_details.result.value, static_cast<ValueType>(failureReturnValues)...);
        return PosixCall_Evaluator<ReturnType>(m_details);
    }
    
private:
    detail::PosixCall_Details<ReturnType>& m_details;
};

// ==================== PosixCall_Evaluator ====================
// 评估器：提供访问调用结果的接口
template <typename ReturnType>
class PosixCall_Evaluator
{
public:
    // 构造函数：接收调用详情
    PosixCall_Evaluator(detail::PosixCall_Details<ReturnType>& details) noexcept
        : m_details{details}
    {
    }
    
    // 判断调用是否成功
    inline bool hasSuccess() const noexcept
    {
        return m_details.hasSuccess;
    }
    
    // 获取返回值
    inline ReturnType getValue() const noexcept
    {
        return m_details.result.value;
    }
    
    // 获取错误码
    inline int32_t getErrnum() const noexcept
    {
        return m_details.result.errnum;
    }
    
private:
    detail::PosixCall_Details<ReturnType>& m_details;
};

// ==================== PosixCall_Builder ====================
// 系统构建器：执行 POSIX 调用并自动处理 EINTR，返回 Verifier 进行验证
template <typename ReturnType, typename... Arguments>
class PosixCall_Builder
{
    using FunctionType_t = ReturnType (*)(Arguments...);

public:
    // 构造函数
    PosixCall_Builder(ReturnType (*ZeroCp_PosixCall)(Arguments...),
                      const char* function_name,
                      const char* file_name,
                      int line_number,
                      const char* pretty_function_name) noexcept
        : M_Zercp_PosixCall(ZeroCp_PosixCall),
          m_details{function_name, file_name, line_number, pretty_function_name} 
    {
    }
    
    // 调用操作符：接受函数参数，返回 Verifier
    PosixCall_Verifier<ReturnType> operator()(Arguments... arguments) && noexcept
    {
        // 最多重试 POSIX_CALL_EINTR_REPETITIONS 次来处理 EINTR 中断
        for(uint32_t i = 0; i < POSIX_CALL_EINTR_REPETITIONS; ++i)
        {
            // 调用实际的 POSIX 函数
            m_details.result.value = M_Zercp_PosixCall(arguments...);
            // 保存 errno（系统调用后立即获取）
            m_details.result.errnum = errno;
            
            // 如果不是 EINTR 错误，说明调用完成（成功或其他错误）
            if(m_details.result.errnum != EINTR)
            {
                break;  // 跳出重试循环
            }
            // 如果是 EINTR，继续下一次循环重试
        }
        
        // 返回 Verifier 对象用于后续验证
        return PosixCall_Verifier<ReturnType>(m_details);
    }

private:
    FunctionType_t M_Zercp_PosixCall;
    detail::PosixCall_Details<ReturnType> m_details;
};

}  // namespace ZeroCp

// 调用构造去使用builder 去启用
#define ZeroCp_PosixCall(function) \
    ZeroCp::detail::CreatePosixCall_Builder(&function, \
                                            (#function), \
                                            __FILE__, \
                                            __LINE__, \
                                            __PRETTY_FUNCTION__)

#endif  // POSIX_CALL_HPP
