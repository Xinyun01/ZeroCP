#ifndef PUBLISHER_HPP
#define PUBLISHER_HPP

#include "service_description.hpp"
#include "popo/message_header.hpp"
#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "zerocp_daemon/mempool/shared_chunk.hpp"
#include "zerocp_daemon/memory/include/mempool.hpp"
#include "zerocp_daemon/memory/include/mempool_manager.hpp"

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <new>
#include <unistd.h>

namespace ZeroCP
{
namespace Popo
{

enum class PublisherError
{
    RuntimeUnavailable,
    RegistrationFailed,
    AlreadyOffered,
    NotOffered,
    MemoryManagerUnavailable,
    LoanFailed,
    ChunkReservationFailed,
    RouteFailed,
    AlreadyPublished
};

template<typename T>
class Publisher
{
public:
    class LoanedSample;
    using LoanResult = std::expected<LoanedSample, PublisherError>;
    using PublishResult = std::expected<void, PublisherError>;

    explicit Publisher(const ServiceDescription& serviceDescription) noexcept;

    bool offer() noexcept;
    void stopOffer() noexcept;
    bool isOffered() const noexcept { return m_isOffered; }

    LoanResult loan() noexcept;

private:
    friend class LoanedSample;

    PublishResult finalizeLoan(LoanedSample& sample) noexcept;
    bool ensureRuntime() noexcept;
    Memory::MemPoolManager* ensureMemPoolManager() noexcept;
    PublishResult routeChunk(uint64_t poolId, uint32_t chunkManagerIndex) noexcept;

private:
    ServiceDescription m_serviceDescription;
    Runtime::PoshRuntime* m_runtime{nullptr};
    Memory::MemPoolManager* m_memPoolManager{nullptr};
    bool m_isOffered{false};
};

template<typename T>
class Publisher<T>::LoanedSample
{
public:
    LoanedSample(const LoanedSample&) = delete;
    LoanedSample& operator=(const LoanedSample&) = delete;

    LoanedSample(LoanedSample&& other) noexcept;
    LoanedSample& operator=(LoanedSample&& other) noexcept;

    ~LoanedSample() noexcept;

    T* operator->() noexcept { return m_payload; }
    T& operator*() noexcept { return *m_payload; }
    const T* operator->() const noexcept { return m_payload; }
    const T& operator*() const noexcept { return *m_payload; }

    PublishResult publish() noexcept;

private:
    friend class Publisher<T>;

    LoanedSample(Publisher<T>* owner, Memory::SharedChunk&& chunk, T* payload) noexcept;

    void invalidatePayload() noexcept { m_payload = nullptr; }

private:
    Publisher<T>* m_owner{nullptr};
    Memory::SharedChunk m_chunk;
    T* m_payload{nullptr};
    bool m_published{false};
};

/// ==================== Publisher implementation ====================

template<typename T>
Publisher<T>::Publisher(const ServiceDescription& serviceDescription) noexcept
    : m_serviceDescription(serviceDescription)
{
}

template<typename T>
bool Publisher<T>::ensureRuntime() noexcept
{
    if (m_runtime != nullptr)
    {
        return true;
    }

    m_runtime = &Runtime::PoshRuntime::getInstance();
    return m_runtime != nullptr && m_runtime->isConnected();
}

template<typename T>
Memory::MemPoolManager* Publisher<T>::ensureMemPoolManager() noexcept
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
        ZEROCP_LOG(Error, "Publisher failed to attach to shared MemPoolManager");
        return nullptr;
    }

    m_memPoolManager = Memory::MemPoolManager::getInstanceIfInitialized();
    return m_memPoolManager;
}

template<typename T>
bool Publisher<T>::offer() noexcept
{
    if (m_isOffered)
    {
        return true;
    }

    if (!ensureRuntime())
    {
        ZEROCP_LOG(Error, "Publisher offer failed: runtime unavailable");
        return false;
    }

    std::ostringstream oss;
    oss << "PUBLISHER:"
        << m_runtime->getRuntimeName().c_str() << ":"
        << ::getpid() << ":"
        << m_serviceDescription.getService().c_str() << ":"
        << m_serviceDescription.getInstance().c_str() << ":"
        << m_serviceDescription.getEvent().c_str();

    std::string response;
    if (!m_runtime->requestReply(oss.str(), response))
    {
        ZEROCP_LOG(Error, "Publisher registration request failed");
        return false;
    }

    if (response.rfind("OK:", 0) != 0)
    {
        ZEROCP_LOG(Error, "Publisher registration rejected: " << response);
        return false;
    }

    m_isOffered = true;
    ZEROCP_LOG(Info, "Publisher offered for " 
               << m_serviceDescription.getService().c_str() << "/"
               << m_serviceDescription.getInstance().c_str() << "/"
               << m_serviceDescription.getEvent().c_str());
    return true;
}

template<typename T>
void Publisher<T>::stopOffer() noexcept
{
    m_isOffered = false;
}

template<typename T>
typename Publisher<T>::LoanResult Publisher<T>::loan() noexcept
{
    if (!m_isOffered && !offer())
    {
        return std::unexpected(PublisherError::NotOffered);
    }

    auto* memPoolManager = ensureMemPoolManager();
    if (memPoolManager == nullptr)
    {
        return std::unexpected(PublisherError::MemoryManagerUnavailable);
    }

    constexpr uint64_t payloadSize = static_cast<uint64_t>(sizeof(T));
    Memory::ChunkManager* chunkManager = memPoolManager->getChunk(payloadSize);
    if (chunkManager == nullptr)
    {
        ZEROCP_LOG(Warn, "Publisher loan failed: no chunk available for size " << payloadSize);
        return std::unexpected(PublisherError::LoanFailed);
    }

    Memory::SharedChunk chunk(chunkManager, memPoolManager);
    void* rawPayload = chunk.getUserPayload();
    if (rawPayload == nullptr)
    {
        ZEROCP_LOG(Error, "Publisher loan failed: null payload pointer");
        return std::unexpected(PublisherError::LoanFailed);
    }

    T* payload = nullptr;
    try
    {
        payload = new (rawPayload) T();
    }
    catch (...)
    {
        ZEROCP_LOG(Error, "Publisher loan failed: payload construction threw");
        return std::unexpected(PublisherError::LoanFailed);
    }

    return LoanedSample(this, std::move(chunk), payload);
}

template<typename T>
typename Publisher<T>::PublishResult Publisher<T>::routeChunk(uint64_t poolId,
                                                             uint32_t chunkManagerIndex) noexcept
{
    if (!m_isOffered)
    {
        return std::unexpected(PublisherError::NotOffered);
    }

    if (!ensureRuntime())
    {
        return std::unexpected(PublisherError::RuntimeUnavailable);
    }

    std::ostringstream oss;
    oss << "ROUTE:"
        << m_runtime->getHeartbeatSlotIndex() << ":"
        << m_serviceDescription.getService().c_str() << ":"
        << m_serviceDescription.getInstance().c_str() << ":"
        << m_serviceDescription.getEvent().c_str() << ":"
        << poolId << ":"
        << chunkManagerIndex;

    std::string response;
    if (!m_runtime->requestReply(oss.str(), response))
    {
        ZEROCP_LOG(Error, "Publisher failed to send route message");
        return std::unexpected(PublisherError::RouteFailed);
    }

    if (response.rfind("OK:ROUTED", 0) == 0 || response.rfind("WARN:NO_SUBSCRIBERS", 0) == 0)
    {
        return {};
    }

    ZEROCP_LOG(Error, "Publisher route rejected: " << response);
    return std::unexpected(PublisherError::RouteFailed);
}

template<typename T>
typename Publisher<T>::PublishResult Publisher<T>::finalizeLoan(LoanedSample& sample) noexcept
{
    if (!m_isOffered)
    {
        return std::unexpected(PublisherError::NotOffered);
    }

    auto* chunkManager = sample.m_chunk.get();
    if (chunkManager == nullptr)
    {
        return std::unexpected(PublisherError::LoanFailed);
    }

    auto* dataPool = chunkManager->m_mempool.get();
    if (dataPool == nullptr)
    {
        ZEROCP_LOG(Error, "Publisher finalizeLoan failed: data pool is null");
        return std::unexpected(PublisherError::MemoryManagerUnavailable);
    }

    uint64_t poolId = dataPool->getPoolId();
    uint32_t chunkMgrIndex = sample.m_chunk.getChunkManagerIndex();

    uint32_t reservedIndex = sample.m_chunk.prepareForTransfer();
    if (reservedIndex == UINT32_MAX)
    {
        ZEROCP_LOG(Error, "Publisher finalizeLoan failed: unable to reserve chunk for transfer");
        return std::unexpected(PublisherError::ChunkReservationFailed);
    }

    auto routeResult = routeChunk(poolId, reservedIndex);
    if (!routeResult)
    {
        return routeResult;
    }

    sample.invalidatePayload();
    sample.m_chunk.reset(); // 发布者释放自身引用
    return routeResult;
}

/// ==================== LoanedSample implementation ====================

template<typename T>
Publisher<T>::LoanedSample::LoanedSample(Publisher<T>* owner,
                                         Memory::SharedChunk&& chunk,
                                         T* payload) noexcept
    : m_owner(owner)
    , m_chunk(std::move(chunk))
    , m_payload(payload)
{
}

template<typename T>
Publisher<T>::LoanedSample::LoanedSample(LoanedSample&& other) noexcept
    : m_owner(other.m_owner)
    , m_chunk(std::move(other.m_chunk))
    , m_payload(other.m_payload)
    , m_published(other.m_published)
{
    other.m_owner = nullptr;
    other.m_payload = nullptr;
    other.m_published = true;
}

template<typename T>
typename Publisher<T>::LoanedSample&
Publisher<T>::LoanedSample::operator=(LoanedSample&& other) noexcept
{
    if (this != &other)
    {
        if (!m_published && m_payload != nullptr)
        {
            std::destroy_at(m_payload);
        }

        m_owner = other.m_owner;
        m_chunk = std::move(other.m_chunk);
        m_payload = other.m_payload;
        m_published = other.m_published;

        other.m_owner = nullptr;
        other.m_payload = nullptr;
        other.m_published = true;
    }
    return *this;
}

template<typename T>
Publisher<T>::LoanedSample::~LoanedSample() noexcept
{
    if (!m_published && m_payload != nullptr)
    {
        std::destroy_at(m_payload);
    }
}

template<typename T>
typename Publisher<T>::PublishResult Publisher<T>::LoanedSample::publish() noexcept
{
    if (m_published)
    {
        return std::unexpected(PublisherError::AlreadyPublished);
    }

    if (m_owner == nullptr)
    {
        return std::unexpected(PublisherError::NotOffered);
    }

    auto result = m_owner->finalizeLoan(*this);
    if (result)
    {
        m_published = true;
        m_owner = nullptr;
    }
    return result;
}

} // namespace Popo
} // namespace ZeroCP

#endif