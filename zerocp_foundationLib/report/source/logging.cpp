#include "logging.hpp"
#include "log_backend.hpp"

namespace ZeroCP {
namespace Log {

// 构造函数：创建后端并自动启动
Log_Manager::Log_Manager() noexcept
{
    // 1. 创建 LogBackend 对象
    backend_ = std::make_unique<LogBackend>();
    
    // 2. 启动后台线程
    start();
}

// 析构函数：确保停止后台线程
Log_Manager::~Log_Manager() noexcept
{
    stop();
}

// 启动日志系统（调用后端的 start）
void Log_Manager::start() noexcept
{
    if (backend_) {
        backend_->start();
    }
}

// 停止日志系统（调用后端的 stop）
void Log_Manager::stop() noexcept
{
    if (backend_) {
        backend_->stop();
    }
}

} // namespace Log
} // namespace ZeroCP
