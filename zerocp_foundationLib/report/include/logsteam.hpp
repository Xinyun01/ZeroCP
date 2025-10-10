#ifndef LOGSTREAM_HPP
#define LOGSTREAM_HPP

#include <iostream>
#include <sstream>
#include <string>

namespace ZeroCP
{
namespace Log
{
class LogStream
{
public:

LogStream(const char* file, const int line, const char* function, LogLevel logLevel) noexcept;
LogStream(const LogStream&) = delete;
LogStream(LogStream&&) = delete;

LogStream& operator=(const LogStream&) = delete;
LogStream& operator=(LogStream&&) = delete;



 // Start of Selection
 // 重载运算符以处理C风格字符串
 LogStream& operator<<(const char* str) noexcept;

 // 重载运算符以处理std::string类型
 LogStream& operator<<(const std::string& str) noexcept;

 // 重载运算符以处理int类型
 LogStream& operator<<(const int& value) noexcept;

 // 重载运算符以处理double类型
 LogStream& operator<<(const double& value) noexcept;

 // 重载运算符以处理bool类型
 LogStream& operator<<(const bool& value) noexcept;

 // 重载运算符以处理char类型
 LogStream& operator<<(const char& value) noexcept;

 // 重载运算符以处理unsigned char类型
 LogStream& operator<<(const unsigned char& value) noexcept;

 // 重载运算符以处理short类型
 LogStream& operator<<(const short& value) noexcept;

 // 重载运算符以处理unsigned short类型
 LogStream& operator<<(const unsigned short& value) noexcept;

 // 重载运算符以处理long类型
 LogStream& operator<<(const long& value) noexcept;

 // 重载运算符以处理unsigned long类型
 LogStream& operator<<(const unsigned long& value) noexcept;

 // 重载运算符以处理long long类型
 LogStream& operator<<(const long long& value) noexcept;

 // 重载运算符以处理unsigned long long类型
 LogStream& operator<<(const unsigned long long& value) noexcept;


~LogStream();

LogStream& self() noexcept;


}
}
}

#endif