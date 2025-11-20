#include "unix_domainsocket.hpp"      // UDS相关头文件
#include <unistd.h>                  // POSIX close、unlink等函数
#include <cerrno>                    // errno定义
#include <cstring>                   // strncpy等字符串操作
#include <vector>                    // std::vector (用于动态缓冲区)
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
    
    // int socket(int domain,       // 地址族：AF_UNIX(本地通信), AF_INET(IPv4), AF_INET6(IPv6)
    //            int type,         // 类型：SOCK_STREAM(流式), SOCK_DGRAM(数据报), SOCK_RAW(原始)
    //            int protocol);    // 协议：通常为0(自动选择)
    // 返回值：成功返回文件描述符(>=0)，失败返回-1并设置errno
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
    
    // 检查路径长度，防止截断
    if (m_name.size() >= sizeof(addr.sun_path))
    {
        closeFileDescriptor(m_name, sockfd, addr, m_channelSide);
        return std::unexpected(PosixIpcChannelError::INVALID_ARGUMENTS);
    }
    
    // 使用 memcpy 替代 strncpy，避免编译器警告
    // 因为我们已经检查了长度，这是安全的
    std::memcpy(addr.sun_path, m_name.c_str(), m_name.size());
    addr.sun_path[m_name.size()] = '\0';  // 手动添加 null 终止符

    // int unlink(const char *pathname);  // 要删除的文件路径
    // 功能：删除文件系统中的文件名，当链接计数为0时删除文件
    // 返回值：成功返回0，失败返回-1并设置errno
    // 常见errno：ENOENT(文件不存在), EACCES(权限不足), EISDIR(是目录)
    ZeroCp_PosixCall(unlink)(m_name.c_str())
        .failureReturnValue(UnixDomainSocket::ERROR_CODE)
        .ignoreErrnos(ENOENT)
        .evaluate();
    
    // int bind(int sockfd,                    // socket文件描述符
    //          const struct sockaddr *addr,  // 要绑定的地址结构（对于UNIX域socket是sockaddr_un）
    //          socklen_t addrlen);           // 地址结构的长度
    // 功能：将socket绑定到指定地址（对于UNIX域socket，在文件系统创建socket文件）
    // 返回值：成功返回0，失败返回-1并设置errno
    // 常见errno：EADDRINUSE(地址已被使用), EACCES(权限不足), ENOENT(目录不存在)
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




// ssize_t recvfrom(int sockfd, 
//     void *buf,           // 接收缓冲区
//     size_t len,          // 缓冲区大小
//     int flags,           // 标志
//     struct sockaddr *src_addr,  // ⭐ 发送方地址（输出参数）
//     socklen_t *addrlen);        // ⭐ 地址长度（输入/输出参数）
/**
 * @brief 从socket接收数据到msg，fromAddr填充来源地址
 */
std::expected<std::string, PosixIpcChannelError> UnixDomainSocket::receiveFrom(
    std::string& msg,
    sockaddr_un& fromAddr) const noexcept
{
    // 使用 vector 替代 VLA，避免编译警告
    std::vector<char> buffer(m_maxMsgSize);
    socklen_t fromLen = sizeof(fromAddr);
    
    auto recvResult = ZeroCp_PosixCall(recvfrom)(m_socketFd,
                                                  buffer.data(),
                                                  buffer.size() - 1,
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
    msg = std::string(buffer.data(), bytesReceived);  // 拷贝到输出参数
    
    return msg;
}



// ssize_t sendto(int sockfd,                    // 套接字文件描述符
//                const void *buf,               // 要发送的数据缓冲区
//                size_t len,                    // 数据长度
//                int flags,                     // 标志位（通常为 0）
//                const struct sockaddr *dest_addr,  // ⭐ 目标地址
//                socklen_t addrlen);            // 地址结构体长度

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
 * @brief 设置socket接收超时时间，使recvfrom可被周期性中断
 */
std::expected<void, PosixIpcChannelError> UnixDomainSocket::setReceiveTimeout(uint32_t timeoutMs) noexcept
{
    if (m_socketFd == INVALID_FD)
    {
        ZEROCP_LOG(Error, "Cannot set timeout on invalid socket");
        return std::unexpected(PosixIpcChannelError::INVALID_FILE_DESCRIPTOR);
    }
    
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;           // 秒
    tv.tv_usec = (timeoutMs % 1000) * 1000; // 微秒
    
    // setsockopt() 设置socket选项
    // SOL_SOCKET: socket层级选项
    // SO_RCVTIMEO: 接收超时选项
    auto result = ZeroCp_PosixCall(setsockopt)(m_socketFd,
                                                SOL_SOCKET,
                                                SO_RCVTIMEO,
                                                &tv,
                                                sizeof(tv))
        .failureReturnValue(ERROR_CODE)
        .evaluate();
    
    if (!result.has_value())
    {
        ZEROCP_LOG(Error, "Failed to set receive timeout: errno=" << result.error().errnum);
        return std::unexpected(errnoToEnum(m_name, result.error().errnum));
    }
    
    ZEROCP_LOG(Info, "Socket receive timeout set to " << timeoutMs << "ms for: " << m_name.c_str());
    return {};
}

/**
 * @brief errno转为枚举类型错误码
 */
PosixIpcChannelError UnixDomainSocket::errnoToEnum([[maybe_unused]] const UdsName_t& name, int32_t errnum) noexcept
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
        case EAGAIN:        // recvfrom 超时会返回 EAGAIN (注: EWOULDBLOCK 在某些系统上与 EAGAIN 相同)
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
    [[maybe_unused]] const sockaddr_un& sockAddr,
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
