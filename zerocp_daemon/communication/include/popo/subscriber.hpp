#ifndef SUBSCRIBER_HPP
#define SUBSCRIBER_HPP

#include "service_description.hpp"
#include "popo/message_header.hpp"
#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/lockfree_ringbuffer.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "zerocp_daemon/mempool/shared_chunk.hpp"
#include "zerocp_daemon/memory/include/mempool_manager.hpp"

#include <expected>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <unistd.h>

namespace ZeroCP
{
namespace Popo
{

enum class SubscribeError
{
    RuntimeUnavailable,
    RegistrationFailed,
    SharedMemoryUnavailable
};

enum class ChunkReceiveResult
{
    NotSubscribed,
    QueueUnavailable,
    NoChunkAvailable,
    MemoryManagerUnavailable,
    InvalidChunkHandle
};

template<typename T>
class Subscriber
{
public:
    class Sample;

    explicit Subscriber(const ServiceDescription& serviceDescription) noexcept;

    bool subscribe() noexcept;
    void unsubscribe() noexcept;
    bool isSubscribed() const noexcept { return m_isSubscribed; }

    std::expected<Sample, ChunkReceiveResult> take() noexcept;

private:
    using MessageQueue = ZeroCP::Log::LockFreeRingBuffer<Popo::MessageHeader, 1024>;

    bool ensureRuntime() noexcept;
    Memory::MemPoolManager* ensureMemPoolManager() noexcept;
    bool mapReceiveQueue() noexcept;

private:
    ServiceDescription m_serviceDescription;
    Runtime::PoshRuntime* m_runtime{nullptr};
    Memory::MemPoolManager* m_memPoolManager{nullptr};
    bool m_isSubscribed{false};
    uint64_t m_queueOffset{0};
    void* m_sharedMemoryBase{nullptr};
    MessageQueue* m_queue{nullptr};
};

template<typename T>
class Subscriber<T>::Sample
{
public:
    Sample(const Sample&) = delete;
    Sample& operator=(const Sample&) = delete;
    Sample(Sample&&) noexcept = default;
    Sample& operator=(Sample&&) noexcept = default;
    ~Sample() = default;

    const T* operator->() const noexcept { return m_payload; }
    const T& operator*() const noexcept { return *m_payload; }
    const Popo::MessageHeader& header() const noexcept { return m_header; }

private:
    friend class Subscriber<T>;

    Sample(Memory::SharedChunk&& chunk, const Popo::MessageHeader& header) noexcept
        : m_chunk(std::move(chunk))
        , m_payload(static_cast<const T*>(m_chunk.getUserPayload()))
        , m_header(header)
    {
    }

    Memory::SharedChunk m_chunk;
    const T* m_payload{nullptr};
    Popo::MessageHeader m_header{};
};

template<typename T>
Subscriber<T>::Subscriber(const ServiceDescription& serviceDescription) noexcept
    : m_serviceDescription(serviceDescription)
{
}

template<typename T>
bool Subscriber<T>::ensureRuntime() noexcept
{
    if (m_runtime != nullptr)
    {
        return true;
    }

    m_runtime = &Runtime::PoshRuntime::getInstance();
    return m_runtime != nullptr && m_runtime->isConnected();
}

template<typename T>
Memory::MemPoolManager* Subscriber<T>::ensureMemPoolManager() noexcept
{
    if (m_memPoolManager != nullptr)
    {
        return m_memPoolManager;
    }

    m_memPoolManager = Memory::MemPoolManager::getInstanceIfInitialized();
    if (m_memPoolManager)
    {
        return m_memPoolManager;
    }

    if (!Memory::MemPoolManager::attachToSharedInstance())
    {
        ZEROCP_LOG(Error, "Subscriber failed to attach to shared MemPoolManager");
        return nullptr;
    }

    m_memPoolManager = Memory::MemPoolManager::getInstanceIfInitialized();
    return m_memPoolManager;
}

template<typename T>
bool Subscriber<T>::mapReceiveQueue() noexcept
{
    if (m_queue != nullptr)
    {
        return true;
    }

    if (!ensureRuntime())
    {
        return false;
    }

    void* baseAddress = m_runtime->getSharedMemoryBaseAddress();
    if (baseAddress == nullptr)
    {
        ZEROCP_LOG(Error, "Subscriber cannot access Diroute shared memory");
        return false;
    }

    m_sharedMemoryBase = baseAddress;
    m_queue = reinterpret_cast<MessageQueue*>(static_cast<char*>(baseAddress) + m_queueOffset);
    return true;
}

template<typename T>
bool Subscriber<T>::subscribe() noexcept
{
    if (m_isSubscribed)
    {
        return true;
    }

    if (!ensureRuntime())
    {
        ZEROCP_LOG(Error, "Subscriber subscribe failed: runtime unavailable");
        return false;
    }

    std::ostringstream oss;
    oss << "SUBSCRIBER:"
        << m_runtime->getRuntimeName().c_str() << ":"
        << ::getpid() << ":"
        << m_serviceDescription.getService().c_str() << ":"
        << m_serviceDescription.getInstance().c_str() << ":"
        << m_serviceDescription.getEvent().c_str();

    std::string response;
    if (!m_runtime->requestReply(oss.str(), response))
    {
        ZEROCP_LOG(Error, "Subscriber registration request failed");
        return false;
    }

    constexpr std::string_view OFFSET_TAG = "QUEUE_OFFSET:";
    auto pos = response.find(OFFSET_TAG.data());
    if (pos == std::string::npos)
    {
        ZEROCP_LOG(Error, "Subscriber registration response missing queue offset: " << response);
        return false;
    }

    try
    {
        m_queueOffset = std::stoull(response.substr(pos + OFFSET_TAG.size()));
    }
    catch (...)
    {
        ZEROCP_LOG(Error, "Subscriber failed to parse queue offset from: " << response);
        return false;
    }

    if (!mapReceiveQueue())
    {
        ZEROCP_LOG(Error, "Subscriber failed to map receive queue");
        return false;
    }

    m_isSubscribed = true;
    ZEROCP_LOG(Info, "Subscriber registered for "
               << m_serviceDescription.getService().c_str() << "/"
               << m_serviceDescription.getInstance().c_str() << "/"
               << m_serviceDescription.getEvent().c_str()
               << " queueOffset=" << m_queueOffset);
    return true;
}

template<typename T>
void Subscriber<T>::unsubscribe() noexcept
{
    m_isSubscribed = false;
}

template<typename T>
std::expected<typename Subscriber<T>::Sample, ChunkReceiveResult>
Subscriber<T>::take() noexcept
{
    if (!m_isSubscribed)
    {
        return std::unexpected(ChunkReceiveResult::NotSubscribed);
    }

    if (m_queue == nullptr && !mapReceiveQueue())
    {
        return std::unexpected(ChunkReceiveResult::QueueUnavailable);
    }

    Popo::MessageHeader header;
    if (!m_queue->tryPop(header))
    {
        return std::unexpected(ChunkReceiveResult::NoChunkAvailable);
    }

    auto* memPoolManager = ensureMemPoolManager();
    if (memPoolManager == nullptr)
    {
        return std::unexpected(ChunkReceiveResult::MemoryManagerUnavailable);
    }

    uint32_t chunkManagerIndex = static_cast<uint32_t>(header.chunk.chunkOffset);
    auto chunk = Memory::SharedChunk::fromIndex(chunkManagerIndex, memPoolManager);
    if (!chunk.isValid())
    {
        return std::unexpected(ChunkReceiveResult::InvalidChunkHandle);
    }

    return Sample(std::move(chunk), header);
}

} // namespace Popo
} // namespace ZeroCP

#endif

