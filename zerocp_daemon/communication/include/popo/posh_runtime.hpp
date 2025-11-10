#ifndef POSH_RUNTIME_HPP
#define POSH_RUNTIME_HPP

#include "service_description.hpp"

namespace ZeroCP
{
class PoshRuntime
{
public:
    explicit PoshRuntime(ServiceDescription& serviceDescription) noexcept;
}
}