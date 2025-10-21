#ifndef UNIX_DOMAIN_SOCKET_HPP
#define UNIX_DOMAIN_SOCKET_HPP

#include "filesystem.hpp"
#include "builder.hpp"
#include <expected>
#include "string.hpp"
#include <cstdint>
#include <sys/socket.h>
#include <sys/un.h>
#include "logging.hpp"

namespace ZeroCP
{

/// @brief POSIX IPC 通道错误类型
enum class PosixIpcChannelError : uint8_t
{
    INVALID_CHANNEL_NAME,         // 通道名称无效
    CHANNEL_NAME_TOO_LONG,        // 通道名称过长
    SOCKET_CREATION_FAILED,       // socket() 调用失败
    BIND_FAILED,                  // bind() 调用失败
    LISTEN_FAILED,                // listen() 调用失败
    CONNECT_FAILED,               // connect() 调用失败
    ACCEPT_FAILED,                // accept() 调用失败
    SEND_FAILED,                  // send() 调用失败
    RECEIVE_FAILED,               // recv() 调用失败
    INSUFFICIENT_PERMISSIONS,     // 权限不足
    ADDRESS_IN_USE,               // 地址已被使用
    INVALID_FILE_DESCRIPTOR,      // 无效的文件描述符
    INVALID_ARGUMENTS,            // 无效的参数
    DOES_NOT_EXIST,               // 不存在
    CONNECTION_REFUSED,           // 连接被拒绝
    ALREADY_CONNECTED,            // 已经连接
    UNREACHABLE,                  // 不可达
    TIMEOUT,                      // 超时
    TOO_MANY_FILES,               // 打开文件过多
    NO_MEMORY,                    // 内存不足
    INTERNAL_LOGIC_ERROR,         // 内部逻辑错误
    INVALID_MAX_MESSAGE_SIZE,     // 无效的最大消息大小
    UNKNOWN_ERROR
};

/// @brief POSIX IPC 通道端类型
enum class PosixIpcChannelSide : uint8_t
{
    CLIENT,     // 客户端
    SERVER      // 服务端
};

/// @brief Unix Domain Socket 错误类型（保留用于向后兼容）
enum class UnixDomainSocketError : uint8_t
{
    INVALID_PATH,              // 路径无效
    PATH_TOO_LONG,            // 路径过长（超过 108 字节）
    SOCKET_CREATION_FAILED,   // socket() 调用失败
    BIND_FAILED,              // bind() 调用失败
    LISTEN_FAILED,            // listen() 调用失败
    CONNECT_FAILED,           // connect() 调用失败
    ACCEPT_FAILED,            // accept() 调用失败
    SEND_FAILED,              // send() 调用失败
    RECEIVE_FAILED,           // recv() 调用失败
    ALREADY_BOUND,            // 已经绑定
    NOT_LISTENING,            // 未处于监听状态
    NOT_CONNECTED,            // 未连接
    UNKNOWN_ERROR
};

/// @brief Socket 类型
enum class SocketType : uint8_t
{
    Stream,     // SOCK_STREAM (类似 TCP，面向连接，可靠)
    Datagram    // SOCK_DGRAM (类似 UDP，无连接)
};

enum class UnixDomainSocketSide : uint8_t
{
    CLIENT,
    SERVER
};

/// @brief POSIX IPC 通道名称类型
using PosixIpcChannelName_t = string<108>;  // 最大 108 字节，减去 null terminator

namespace Details
{

class UnixDomainSocket
{
public:
        static constexpr uint64_t MAX_MESSAGE_SIZE = 256;
        static constexpr uint64_t MAX_MESSAGE_NUM = 10;
        using UdsName_t = string<MAX_MESSAGE_SIZE-1>;

        
        // ====================================================================
        // 完整API：适用于多客户端服务端（SOCK_DGRAM 无连接模式）
        // ====================================================================
        /// @brief 接收消息并返回发送者地址（用于多客户端服务端）
        /// @param msg 接收到的消息
        /// @param fromAddr 发送者地址（输出参数）
        /// @return 成功返回消息，失败返回错误码
        std::expected<std::string, PosixIpcChannelError> receiveFrom(std::string& msg, sockaddr_un& fromAddr) const noexcept;
        
        /// @brief 发送消息到指定地址（用于多客户端服务端）
        /// @param msg 要发送的消息
        /// @param toAddr 目标地址
        /// @return 成功返回 void，失败返回错误码
        std::expected<void, PosixIpcChannelError> sendTo(const std::string& msg, const sockaddr_un& toAddr) const noexcept;
        
        /// @brief 将 errno 转换为 PosixIpcChannelError
        /// @param name 套接字名称（用于日志）
        /// @param errnum errno 错误码
        /// @return 对应的 PosixIpcChannelError 枚举值
        static PosixIpcChannelError errnoToEnum(const UdsName_t& name, int32_t errnum) noexcept;
       /// @brief UnixDomainSocket 构造函数
       /// @param name         套接字名称
       /// @param channelSide  通道类型（服务端或客户端）
       /// @param sockfd       文件描述符
       /// @param sockAddr     套接字地址结构体
       /// @param maxMsgSize   最大消息长度
        UnixDomainSocket(const UdsName_t& name,const PosixIpcChannelSide channelSide, int32_t sockfd, const sockaddr_un& sockAddr, uint64_t maxMsgSize) noexcept;
        UnixDomainSocket(const UnixDomainSocket&) = delete;
        UnixDomainSocket(UnixDomainSocket&&) noexcept  ;
        UnixDomainSocket& operator=(const UnixDomainSocket&) = delete;
        UnixDomainSocket& operator=(UnixDomainSocket&&) noexcept ;
        ~UnixDomainSocket() noexcept;
        std::expected<void, PosixIpcChannelError> destroy() noexcept;
private:
        friend class UnixDomainSocketBuilder;
        static constexpr int32_t ERROR_CODE = -1;
        static constexpr int32_t INVALID_FD = -1;
        static constexpr size_t LONGEST_VALID_NAME = sizeof(sockaddr_un::sun_path) - 1;
        
        UdsName_t m_name;
        PosixIpcChannelSide m_channelSide {PosixIpcChannelSide::CLIENT};
        int32_t m_socketFd {INVALID_FD};
        mutable sockaddr_un m_sockAddr_un {};  // mutable: receive() 需要更新客户端地址
        uint64_t m_maxMsgSize {MAX_MESSAGE_SIZE};
};

class UnixDomainSocketBuilder
{
    /// @brief 套接字名称
    ZeroCP_Builder_Implementation(UnixDomainSocket::UdsName_t, name, UnixDomainSocket::UdsName_t())

    /// @brief 客户端或服务端
    ZeroCP_Builder_Implementation(PosixIpcChannelSide, channelSide, PosixIpcChannelSide::CLIENT)

    /// @brief 套接字最大消息长度
    ZeroCP_Builder_Implementation(uint64_t, maxMsgSize, UnixDomainSocket::MAX_MESSAGE_SIZE)

    /// @brief 最大消息数
    ZeroCP_Builder_Implementation(uint64_t, maxMsgNumber, UnixDomainSocket::MAX_MESSAGE_NUM)

    public:
    /// @brief 接受 const char* 类型的 name 重载
    /// @param value C 风格字符串
    /// @return 当前对象右值引用

    /// @brief 创建一个unix域套接字（builder接口）
    std::expected<UnixDomainSocket, PosixIpcChannelError> create() const noexcept;
    
    /// @brief 关闭文件描述符并清理资源
    static std::expected<void, PosixIpcChannelError> closeFileDescriptor(const UnixDomainSocket::UdsName_t& name,
                                                                    const int sockfd,
                                                                    const sockaddr_un& sockAddr,
                                                                    PosixIpcChannelSide channelSide) noexcept;
};


} // namespace Details
} // namespace ZeroCP

#endif // UNIX_DOMAIN_SOCKET_HPP
