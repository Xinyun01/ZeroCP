#include "unix_domainsocket.hpp"      // UDS相关头文件
#include <unistd.h>                  // POSIX close、unlink等函数
#include <cerrno>                    // errno定义
#include <cstring>                   // strncpy等字符串操作
#include <sys/socket.h>              // socket/bind相关
#include <sys/un.h>                  // UNIX域socket相关
#include "logging.hpp"               // 日志接口
#include "posix_call.hpp"            // 包装POSIX调用
namespace ZeroCP
{
namespace Details
{

// ============================================================================
// UnixDomainSocketBuilder 实现
// ============================================================================

/**
 * @brief 创建UnixDomainSocket实例
 * 
 * 验证名称有效并创建socket，绑定到指定路径
 */
std::expected<UnixDomainSocket, PosixIpcChannelError> UnixDomainSocketBuilder::create() const noexcept
{
    // 验证socket名称不能为空
    if (m_name.size() == 0)
    {
        ZEROCP_LOG(Error, "UnixDomainSocketBuilder::create() failed: name is empty");
        return std::unexpected(PosixIpcChannelError::INVALID_CHANNEL_NAME);
    }
    
    // 验证名称不能太长
    if (m_name.size() >= UnixDomainSocket::LONGEST_VALID_NAME)
    {
        ZEROCP_LOG(Error, "UnixDomainSocketBuilder::create() failed: name is too long");
        return std::unexpected(PosixIpcChannelError::CHANNEL_NAME_TOO_LONG);
    }
    
    // 创建UNIX域DGRAM socket
    auto socketResult = ZeroCp_PosixCall(socket)(AF_UNIX, SOCK_DGRAM, 0)
        .failureReturnValue(UnixDomainSocket::ERROR_CODE)
        .evaluate();
    
    // 检查socket创建是否成功
    if (!socketResult.has_value())
    {
        ZEROCP_LOG(Error, "UnixDomainSocketBuilder::create() failed: socket creation failed");
        return std::unexpected(UnixDomainSocket::errnoToEnum(m_name, socketResult.error().errnum));
    }
    
    int sockfd = socketResult.value().value;
    
    // 填写UNIX socket地址结构
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, m_name.c_str(), sizeof(addr.sun_path) - 1);

    // 删除老的socket文件（忽略不存在错误）
    ZeroCp_PosixCall(unlink)(m_name.c_str())
        .failureReturnValue(UnixDomainSocket::ERROR_CODE)
        .ignoreErrnos(ENOENT)
        .evaluate();
    
    // 绑定socket到文件路径
    auto bindResult = ZeroCp_PosixCall(bind)(sockfd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr))
        .failureReturnValue(UnixDomainSocket::ERROR_CODE)
        .evaluate();
    
    // 检查bind是否成功
    if (!bindResult.has_value())
    {
        ZEROCP_LOG(Error, "UnixDomainSocketBuilder::create() failed: bind failed, errno=" << bindResult.error().errnum);
        ZeroCp_PosixCall(close)(sockfd).failureReturnValue(UnixDomainSocket::ERROR_CODE).evaluate();
        return std::unexpected(UnixDomainSocket::errnoToEnum(m_name, bindResult.error().errnum));
    }
    
    // 打印绑定日志
    const char* roleStr = (m_channelSide == PosixIpcChannelSide::SERVER) ? "Server" : "Client";
    ZEROCP_LOG(Info, roleStr << " socket bound to: " << m_name.c_str());

    // 返回创建的UnixDomainSocket对象
    return UnixDomainSocket(m_name, m_channelSide, sockfd, addr, m_maxMsgSize);
}


// ============================================================================
// UnixDomainSocket 实现
// ============================================================================

/**
 * @brief 构造函数，初始化Unix域socket对象
 */
UnixDomainSocket::UnixDomainSocket(const UdsName_t& name,
                                   const PosixIpcChannelSide channelSide,
                                   int32_t sockfd,
                                   const sockaddr_un& sockAddr,
                                   uint64_t maxMsgSize) noexcept
    : m_name(name)
    , m_channelSide(channelSide)
    , m_socketFd(sockfd)
    , m_sockAddr_un(sockAddr)
    , m_maxMsgSize(maxMsgSize)
{
}

/**
 * @brief 移动构造函数
 */
UnixDomainSocket::UnixDomainSocket(UnixDomainSocket&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_channelSide(other.m_channelSide)
    , m_socketFd(other.m_socketFd)
    , m_sockAddr_un(other.m_sockAddr_un)
    , m_maxMsgSize(other.m_maxMsgSize)
{
    other.m_socketFd = INVALID_FD; // 防止重复释放
}

/**
 * @brief 移动赋值运算符
 */
UnixDomainSocket& UnixDomainSocket::operator=(UnixDomainSocket&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        
        m_name = std::move(other.m_name);
        m_channelSide = other.m_channelSide;
        m_socketFd = other.m_socketFd;
        m_sockAddr_un = other.m_sockAddr_un;
        m_maxMsgSize = other.m_maxMsgSize;
        
        other.m_socketFd = INVALID_FD;
    }
    return *this;
}

/**
 * @brief 析构函数
 */
UnixDomainSocket::~UnixDomainSocket() noexcept
{
    destroy();
}

/**
 * @brief 销毁并关闭socket，服务端额外移除socket文件
 */
std::expected<void, PosixIpcChannelError> UnixDomainSocket::destroy() noexcept
{
    if (m_socketFd != INVALID_FD)
    {
        // 关闭socket
        ZeroCp_PosixCall(close)(m_socketFd)
            .failureReturnValue(ERROR_CODE)
            .evaluate();
        m_socketFd = INVALID_FD;
        
        // 只有服务端负责删除socket文件
        if (m_channelSide == PosixIpcChannelSide::SERVER)
        {
            ZeroCp_PosixCall(unlink)(m_name.c_str())
                .failureReturnValue(ERROR_CODE)
                .ignoreErrnos(ENOENT)
                .evaluate();
        }
    }
    return {};
}

/**
 * @brief 从socket接收数据到msg，fromAddr填充来源地址
 */
std::expected<std::string, PosixIpcChannelError> UnixDomainSocket::receiveFrom(
    std::string& msg,
    sockaddr_un& fromAddr) const noexcept
{
    char buffer[m_maxMsgSize];                 // 用于接收数据的缓冲区
    socklen_t fromLen = sizeof(fromAddr);
    
    auto recvResult = ZeroCp_PosixCall(recvfrom)(m_socketFd,
                                                  buffer,
                                                  sizeof(buffer) - 1,
                                                  0,
                                                  reinterpret_cast<sockaddr*>(&fromAddr),
                                                  &fromLen)
        .failureReturnValue(ERROR_CODE)
        .evaluate();
    
    // 检查接收结果
    if (!recvResult.has_value())
    {
        return std::unexpected(errnoToEnum(m_name, recvResult.error().errnum));
    }
    
    ssize_t bytesReceived = recvResult.value().value;
    buffer[bytesReceived] = '\0';              // 保证字符串结尾
    msg = std::string(buffer, bytesReceived);  // 拷贝到输出参数
    
    return msg;
}

/**
 * @brief 向toAddr发送msg数据
 */
std::expected<void, PosixIpcChannelError> UnixDomainSocket::sendTo(
    const std::string& msg,
    const sockaddr_un& toAddr) const noexcept
{
    auto sendResult = ZeroCp_PosixCall(sendto)(m_socketFd,
                                                msg.c_str(),
                                                msg.length(),
                                                0,
                                                reinterpret_cast<const sockaddr*>(&toAddr),
                                                sizeof(toAddr))
        .failureReturnValue(ERROR_CODE)
        .evaluate();
    
    // 检查发送结果
    if (!sendResult.has_value())
    {
        return std::unexpected(errnoToEnum(m_name, sendResult.error().errnum));
    }
    
    return {};
}

/**
 * @brief errno转为枚举类型错误码
 */
PosixIpcChannelError UnixDomainSocket::errnoToEnum(const UdsName_t& name, int32_t errnum) noexcept
{
    switch (errnum)
    {
        case EACCES:
        case EPERM:
            return PosixIpcChannelError::INSUFFICIENT_PERMISSIONS;
        case EADDRINUSE:
            return PosixIpcChannelError::ADDRESS_IN_USE;
        case EBADF:
        case ENOTSOCK:
            return PosixIpcChannelError::INVALID_FILE_DESCRIPTOR;
        case EINVAL:
            return PosixIpcChannelError::INVALID_ARGUMENTS;
        case ENOENT:
            return PosixIpcChannelError::DOES_NOT_EXIST;
        case ECONNREFUSED:
            return PosixIpcChannelError::CONNECTION_REFUSED;
        case EISCONN:
            return PosixIpcChannelError::ALREADY_CONNECTED;
        case ENETUNREACH:
        case EHOSTUNREACH:
            return PosixIpcChannelError::UNREACHABLE;
        case ETIMEDOUT:
            return PosixIpcChannelError::TIMEOUT;
        case EMFILE:
        case ENFILE:
            return PosixIpcChannelError::TOO_MANY_FILES;
        case ENOMEM:
        case ENOBUFS:
            return PosixIpcChannelError::NO_MEMORY;
        default:
            return PosixIpcChannelError::UNKNOWN_ERROR;
    }
}


/**
 * @brief 工具函数，关闭fd并如有必要删除socket文件
 */
std::expected<void, PosixIpcChannelError> UnixDomainSocketBuilder::closeFileDescriptor(
    const UnixDomainSocket::UdsName_t& name,
    const int sockfd,
    const sockaddr_un& sockAddr,
    PosixIpcChannelSide channelSide) noexcept
{
    if (sockfd != UnixDomainSocket::INVALID_FD)
    {
        ZeroCp_PosixCall(close)(sockfd)
            .failureReturnValue(UnixDomainSocket::ERROR_CODE)
            .evaluate();
        
        // 只有服务端负责移除socket文件
        if (channelSide == PosixIpcChannelSide::SERVER)
        {
            ZeroCp_PosixCall(unlink)(name.c_str())
                .failureReturnValue(UnixDomainSocket::ERROR_CODE)
                .ignoreErrnos(ENOENT)
                .evaluate();
        }
    }
    
    return {};
}

} // namespace Details
} // namespace ZeroCP
