#ifndef LOCKFREE_RINGBUFFER_HPP
#define LOCKFREE_RINGBUFFER_HPP

#include <atomic>
#include <array>
#include <cstddef>
#include <cstring>
#include <string>

namespace ZeroCP
{
namespace Log
{

/// @brief 日志队列容量（必须是2的幂）
constexpr size_t LOG_QUEUE_CAPACITY = 1024;

/// @brief 日志消息结构
struct LogMessage
{
    static constexpr size_t MAX_MESSAGE_SIZE = 256;  // 最大消息长度
    
    char message[MAX_MESSAGE_SIZE];  // 消息内容
    size_t length{0};                // 实际消息长度
    
    LogMessage() noexcept = default;
    
    /// @brief 拷贝构造函数（优化版：只拷贝实际长度）
    /// @note 显式定义以避免拷贝整个 256 字节，只拷贝实际使用的部分
    LogMessage(const LogMessage& other) noexcept;
    
    /// @brief 拷贝赋值运算符（优化版：只拷贝实际长度）
    /// @note 显式定义以避免拷贝整个 256 字节，只拷贝实际使用的部分
    LogMessage& operator=(const LogMessage& other) noexcept;
    
    /// @brief 设置消息内容
    void setMessage(const std::string& msg) noexcept;
    
    /// @brief 获取消息字符串
    std::string getMessage() const noexcept;
};

/// @brief 无锁环形队列（单生产者多消费者）
/// @tparam T 元素类型
/// @tparam Size 队列大小（必须是2的幂）
/// @note 提示：需要使用 std::atomic 和适当的内存序来实现线程安全
template<typename T, size_t Size>
class LockFreeRingBuffer
{
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    
public:
    LockFreeRingBuffer() noexcept;
    ~LockFreeRingBuffer() noexcept = default;

    // 禁止拷贝和移动
    LockFreeRingBuffer(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer(LockFreeRingBuffer&&) = delete;
    LockFreeRingBuffer& operator=(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer& operator=(LockFreeRingBuffer&&) = delete;

    /// @brief 尝试将元素推入队列
    /// @param item 要推入的元素
    /// @return 成功返回true，队列满返回false
    /// @note 提示：需要检查队列是否已满，并使用适当的内存序更新写索引
    bool tryPush(const T& item) noexcept;

    /// @brief 尝试从队列中弹出元素
    /// @param item 用于接收弹出元素的引用
    /// @return 成功返回true，队列空返回false
    /// @note 提示：需要检查队列是否为空，并使用适当的内存序更新读索引
    bool tryPop(T& item) noexcept;

    /// @brief 检查队列是否为空
    bool isEmpty() const noexcept;

    /// @brief 检查队列是否已满
    bool isFull() const noexcept;

    /// @brief 获取队列中的元素数量（近似值）
    size_t size() const noexcept;

    /// @brief 获取队列容量
    constexpr size_t capacity() const noexcept;

    // ========== 零拷贝接口 ==========
    
    /// @brief 获取写入位置的指针（零拷贝写入）
    /// @return 写入位置的指针，如果队列满返回 nullptr
    /// @note 调用者负责填充数据，之后必须调用 commitPush()
    T* beginPush() noexcept;
    
    /// @brief 提交写入操作
    /// @note 必须在 beginPush() 之后调用
    void commitPush() noexcept;
    
    /// @brief 获取读取位置的指针（零拷贝读取）
    /// @return 读取位置的指针，如果队列空返回 nullptr
    /// @note 调用者负责读取数据，之后必须调用 commitPop()
    const T* beginPop() noexcept;
    
    /// @brief 提交读取操作
    /// @note 必须在 beginPop() 之后调用
    void commitPop() noexcept;

private:
    // 数据成员：使用 cache line 对齐避免伪共享
    alignas(64) std::atomic<size_t> write_index_;  // 写索引
    alignas(64) std::atomic<size_t> read_index_;   // 读索引
    std::array<T, Size> buffer_;                   // 数据缓冲区

};

// ==================== 实现部分 ====================
// 模板实现在 .inl 文件中
#include "lockfree_ringbuffer.inl"

} // namespace Log
} // namespace ZeroCP

#endif // LOCKFREE_RINGBUFFER_HPP

