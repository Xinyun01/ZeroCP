#ifndef LOGGING_HPP
#define LOGGING_HPP
#include "logsteam.hpp"
namespace ZeroCP
{

namespace Log
{

enum Log_Level uint8_t
{
    Off = 0,      // 关闭日志
    Fatal = 1,    // 致命错误（程序无法继续）
    Error = 2,    // 错误（严重但可恢复）
    Warn = 3,     // 警告（非预期但不严重）
    Info = 4,     // 信息（日常用户关心的）
    Debug = 5,    // 调试（开发者关心的）
    Trace = 6     // 追踪（详细调试信息）
}


class Log_Manager{

public:

 bool LogLevelActive(Log_Level level){
    if(level > Trace){
        return false;
    }
    return true;
 }
}

}

}

/// @brief 日志记录宏
/// @param[in] level 用于日志消息的日志级别
/// @param[in] msg_stream 日志消息流；可以使用 '<<' 操作符记录多个项目
#define ZEROCP_LOG(level,msg_stream)                                                                        \
        ZeroCP::Log::LogStream(__FILE__, __LINE__, static_cast<const char*>(__FUNCTION__),level) << msg_stream
#endif