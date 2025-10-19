#include "../posix_call.hpp"

namespace ZeroCp
{

namespace Details
{
//PosixCall_Details  CreatePosixCall_Builder 初始化默认构造函数
template <typename ReturnType, typename... FunctionArguments>
PosixCall_Builder<ReturnType, FunctionArguments...> CreatePosixCall_Builder(
    ReturnType (*ZeroCp_PosixCall)(FunctionArguments...),
    const char* posixFunctionName,
    const char* file,
    const int32_t line,
    const char* callingFunction) noexcept
{
    return PosixCall_Builder<ReturnType, FunctionArguments...>(
        ZeroCp_PosixCall, posixFunctionName, file, line, callingFunction);
}


// 构造函数已在头文件中内联定义，这里不需要再实现

}  // namespace Details

//PosixCall_Builder 初始化构造函数
template <typename ReturnType, typename... FunctionArguments>
inline PosixCall_Builder<ReturnType, FunctionArguments...>::PosixCall_Builder(
                                                            ReturnType (*ZeroCp_PosixCall)(FunctionArguments...),
                                                            const char* posixFunctionName,
                                                            const char* file,
                                                            const int32_t line,
                                                            const char* callingFunction) noexcept
                                                            : m_ZeroCp_PosixCall(ZeroCp_PosixCall), 
                                                            m_details(posixFunctionName, file, line, callingFunction)
{
}


template <typename ReturnType, typename... FunctionArguments>
inline PosixCallVerificator<ReturnType>
PosixCall_Builder<ReturnType, FunctionArguments...>::operator()(FunctionArguments... arguments) && noexcept
{
    for (uint64_t i = 0U; i < POSIX_CALL_EINTR_REPETITIONS; ++i)
    {
        errno = 0;
        m_details.result.value = m_ZeroCp_PosixCall(arguments...);
        m_details.result.errnum = errno;

        if (m_details.result.errnum != EINTR)
        {
            break;
        }
    }

    return PosixCallVerificator<ReturnType>(m_details);
}

//PosixCall_Verificator
template<typename ReturnType>
inline PosixCallVerificator<ReturnType>::PosixCallVerificator(Details::PosixCall_Details<ReturnType>& details) noexcept
    : m_details(details)
{
}

// ==================== 成员函数实现 ====================

template <typename ReturnType>
template <typename... SuccessReturnValues>
inline PosixCallEvaluator<ReturnType>
PosixCallVerificator<ReturnType>::successReturnValue(const SuccessReturnValues... successReturnValues) && noexcept
{
    m_details.hasSuccess = ZeroCP::algorithm::doesContainValue(m_details.result.value, successReturnValues...);

    return PosixCallEvaluator<ReturnType>(m_details);
}

template <typename ReturnType>
template <typename... FailureReturnValues>
inline PosixCallEvaluator<ReturnType>
PosixCallVerificator<ReturnType>::failureReturnValue(const FailureReturnValues... failureReturnValues) && noexcept
{
    using ValueType = decltype(m_details.result.value);
    m_details.hasSuccess =
        !ZeroCP::algorithm::doesContainValue(m_details.result.value, static_cast<ValueType>(failureReturnValues)...);

    return PosixCallEvaluator<ReturnType>(m_details);
}

template <typename ReturnType>
inline PosixCallEvaluator<ReturnType> PosixCallVerificator<ReturnType>::returnValueMatchesErrno() && noexcept
{
    m_details.hasSuccess = m_details.result.value == 0;
    m_details.result.errnum = static_cast<int32_t>(m_details.result.value);

    return PosixCallEvaluator<ReturnType>(m_details);
}


//PosixCallEvaluator
template <typename ReturnType>
inline PosixCallEvaluator<ReturnType>::PosixCallEvaluator(Details::PosixCall_Details<ReturnType>& details) noexcept
    : m_details{details}
{
}

template <typename ReturnType>
template <typename... IgnoredErrnos>
inline PosixCallEvaluator<ReturnType>
PosixCallEvaluator<ReturnType>::ignoreErrnos(const IgnoredErrnos... ignoredErrnos) const&& noexcept
{
    if (!m_details.hasSuccess)
    {
        m_details.hasIgnoredErrno |= ZeroCP::algorithm::doesContainValue(m_details.result.errnum, ignoredErrnos...);
    }

    return *this;
}

template <typename ReturnType>
template <typename... SilentErrnos>
inline PosixCallEvaluator<ReturnType>
PosixCallEvaluator<ReturnType>::suppressErrorMessagesForErrnos(const SilentErrnos... silentErrnos) const&& noexcept
{
    if (!m_details.hasSuccess)
    {
        m_details.hasSilentErrno |= ZeroCP::algorithm::doesContainValue(m_details.result.errnum, silentErrnos...);
    }

    return *this;
}

// C++23: 使用 std::expected 的标准构造方式
// - 成功情况：直接构造 expected，自动使用值类型
// - 失败情况：使用 std::unexpected 包装错误类型
template <typename ReturnType>
inline std::expected<PosixCallResult<ReturnType>, PosixCallResult<ReturnType>>
PosixCallEvaluator<ReturnType>::evaluate() const&& noexcept
{
    if (m_details.hasSuccess || m_details.hasIgnoredErrno)
    {
        // C++23: 成功时直接构造 std::expected，会自动推导为 value 类型
        return std::expected<PosixCallResult<ReturnType>, PosixCallResult<ReturnType>>(m_details.result);
    }

    if (!m_details.hasSilentErrno)
    {
        // TODO: 添加日志功能
        // 当前注释掉 ZeroCp_Log 调用，因为该功能可能尚未实现
        // ZeroCp_Log(Error,
        //         m_details.file << ":" << m_details.line << " { " << m_details.callingFunction << " -> "
        //                        << m_details.posixFunctionName << " }  :::  [ " << m_details.result.errnum << " ]");
    }

    // C++23: 失败时使用 std::unexpected 包装错误值
    // std::unexpected<E> 是一个辅助类模板，用于表示 expected 的错误状态
    return std::unexpected<PosixCallResult<ReturnType>>(m_details.result);
}

}  // namespace ZeroCp
