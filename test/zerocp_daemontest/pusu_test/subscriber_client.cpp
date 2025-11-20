#include "pusu_test_common.hpp"
#include "popo/subscriber.hpp"
#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "zerocp_daemon/memory/include/mempool_manager.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace
{

bool ensureMemPoolAttached() noexcept
{
    if (auto* manager = ZeroCP::Memory::MemPoolManager::getInstanceIfInitialized(); manager != nullptr)
    {
        return true;
    }

    if (!ZeroCP::Memory::MemPoolManager::attachToSharedInstance())
    {
        ZEROCP_LOG(Error, "[Subscriber] Failed to attach to shared MemPoolManager");
        return false;
    }
    return true;
}

} // namespace

int main()
{
    ZEROCP_LOG(Info, "[Subscriber] Starting pub-sub smoke test subscriber");

    ZeroCP::Runtime::RuntimeName_t runtimeName;
    runtimeName.insert(0, "PusuSubscriber");
    auto& runtime = ZeroCP::Runtime::PoshRuntime::initRuntime(runtimeName);

    if (!runtime.isConnected())
    {
        ZEROCP_LOG(Error, "[Subscriber] Runtime connection failed");
        return 1;
    }

    if (!ensureMemPoolAttached())
    {
        return 1;
    }

    auto service = ZeroCP::Test::makeDefaultService();
    ZeroCP::Popo::Subscriber<ZeroCP::Test::RadarSample> subscriber(service);

    if (!subscriber.subscribe())
    {
        ZEROCP_LOG(Error, "[Subscriber] subscribe() failed");
        return 1;
    }

    ZEROCP_LOG(Info, "[Subscriber] Subscription completed, waiting for samples...");

    constexpr auto kMaxWait = std::chrono::seconds(10);
    const auto deadline = std::chrono::steady_clock::now() + kMaxWait;
    uint32_t received = 0;

    while (received < ZeroCP::Test::kDefaultMessageCount &&
           std::chrono::steady_clock::now() < deadline)
    {
        auto sampleResult = subscriber.take();
        if (!sampleResult.has_value())
        {
            const auto error = sampleResult.error();
            if (error == ZeroCP::Popo::ChunkReceiveResult::NoChunkAvailable)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
            ZEROCP_LOG(Error, "[Subscriber] take() failed with error code "
                       << static_cast<int>(error));
            return 1;
        }

        const auto& sample = sampleResult.value();
        ZEROCP_LOG(Info, "[Subscriber] Received sample #" << received
                   << " seq=" << sample->sequence
                   << " payload=" << sample->payload);
        ++received;
    }

    if (received < ZeroCP::Test::kDefaultMessageCount)
    {
        ZEROCP_LOG(Error, "[Subscriber] Timeout waiting for messages, received=" << received);
        return 1;
    }

    ZEROCP_LOG(Info, "[Subscriber] Successfully received "
               << received << " samples");
    return 0;
}

