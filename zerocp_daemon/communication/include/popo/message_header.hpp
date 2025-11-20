#ifndef MESSAGE_HEADER_HPP
#define MESSAGE_HEADER_HPP

#include "service_description.hpp"
#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include <cstdint>
#include <cstddef>

namespace ZeroCP
{
namespace Popo
{
using RuntimeName_t = ZeroCP::string<108>;

/// @brief 消息头结构（存储在共享内存接收队列中）
/// @note 包含 ServiceDescription 和 chunk 指针信息
struct MessageHeader
{
    // ServiceDescription 信息（用于匹配）
    id_string service;
    id_string instance;
    id_string event;
    
    // Chunk 信息（零拷贝传输）
    uint64_t chunkOffset{0};      // Chunk 在共享内存中的偏移量（相对地址）
    uint64_t chunkSize{0};         // Chunk 大小（字节）
    uint64_t payloadSize{0};       // 用户数据大小（chunkSize - headerSize）
    
    // 元数据
    uint64_t sequenceNumber{0};   // 序列号（用于去重和排序）
    uint64_t timestamp{0};        // 时间戳（纳秒）
    
    // 发送者信息
    RuntimeName_t publisherName;  // 发布者进程名称
    
    MessageHeader() noexcept = default;
    
    /// @brief 从 ServiceDescription 构造
    explicit MessageHeader(const ServiceDescription& desc) noexcept
        : service(desc.getService())
        , instance(desc.getInstance())
        , event(desc.getEvent())
    {
    }
    
    /// @brief 检查消息是否有效
    [[nodiscard]] bool isValid() const noexcept
    {
        return chunkOffset > 0 && chunkSize > 0;
    }
};

} // namespace Popo
} // namespace ZeroCP

#endif // MESSAGE_HEADER_HPP

