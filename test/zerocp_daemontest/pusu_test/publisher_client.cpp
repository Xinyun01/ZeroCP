#include "pusu_test_common.hpp"
#include "popo/publisher.hpp"
#include "popo/posh_runtime.hpp"
#include "zerocp_foundationLib/report/include/logging.hpp"
#include "zerocp_daemon/memory/include/mempool_manager.hpp"

#include <chrono>
#include <cstdio>
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
        ZEROCP_LOG(Error, "Publisher failed to attach to shared MemPoolManager");
        return false;
    }
    return true;
}

} // namespace

int main()
{
    ZEROCP_LOG(Info, "[Publisher] Starting pub-sub smoke test publisher");

    ZeroCP::Runtime::RuntimeName_t runtimeName;
    runtimeName.insert(0, "PusuPublisher");
    auto& runtime = ZeroCP::Runtime::PoshRuntime::initRuntime(runtimeName);

    if (!runtime.isConnected())
    {
        ZEROCP_LOG(Error, "[Publisher] Runtime connection failed");
        return 1;
    }

    if (!ensureMemPoolAttached())
    {
        return 1;
    }

    auto service = ZeroCP::Test::makeDefaultService();
    ZeroCP::Popo::Publisher<ZeroCP::Test::RadarSample> publisher(service);

    if (!publisher.offer())
    {
        ZEROCP_LOG(Error, "[Publisher] offer() failed");
        return 1;
    }

    ZEROCP_LOG(Info, "[Publisher] offer() succeeded, start publishing");

    for (uint32_t i = 0; i < ZeroCP::Test::kDefaultMessageCount; ++i)
    {
        auto loanResult = publisher.loan();
        if (!loanResult.has_value())
        {
            ZEROCP_LOG(Error, "[Publisher] loan() failed at sequence " << i);
            return 1;
        }

        auto sample = std::move(*loanResult);
        ZeroCP::Test::RadarSample& data = *sample;
        char text[64];
        std::snprintf(text, sizeof(text), "seq=%u", i);
        ZeroCP::Test::fillSample(data, i, text);

        auto publishResult = sample.publish();
        if (!publishResult.has_value())
        {
            ZEROCP_LOG(Error, "[Publisher] publish() failed at sequence " << i);
            return 1;
        }

        ZEROCP_LOG(Info, "[Publisher] published message #" << i);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ZEROCP_LOG(Info, "[Publisher] Completed publishing "
               << ZeroCP::Test::kDefaultMessageCount << " samples");
    return 0;
}

