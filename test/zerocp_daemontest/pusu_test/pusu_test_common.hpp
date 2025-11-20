#pragma once

#include "service_description.hpp"
#include "zerocp_foundationLib/vocabulary/include/string.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace ZeroCP::Test
{

struct RadarSample
{
    uint64_t sequence{0};
    std::chrono::steady_clock::time_point timestamp{std::chrono::steady_clock::now()};
    char payload[128]{};
};

inline void fillSample(RadarSample& sample, uint64_t sequence, std::string_view text) noexcept
{
    sample.sequence = sequence;
    sample.timestamp = std::chrono::steady_clock::now();
    std::memset(sample.payload, 0, sizeof(sample.payload));
    const auto copySize = std::min(text.size(), sizeof(sample.payload) - 1);
    std::memcpy(sample.payload, text.data(), copySize);
}

inline ServiceDescription makeDefaultService() noexcept
{
    ZeroCP::id_string service;
    ZeroCP::id_string instance;
    ZeroCP::id_string event;

    service.insert(0, "RadarService");
    instance.insert(0, "Front");
    event.insert(0, "PointCloud");

    return ServiceDescription(service, instance, event);
}

inline constexpr uint32_t kDefaultMessageCount = 16;

} // namespace ZeroCP::Test

