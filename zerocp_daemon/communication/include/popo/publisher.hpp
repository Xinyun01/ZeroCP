#ifndef PUBLISHER_HPP
#define PUBLISHER_HPP

#include "service_description.hpp"

namespace ZeroCP
{
   
template<typename T>
class Publisher
{

public:
    explicit Publisher(ServiceDescription& serviceDescription) noexcept;

private:
};

}

#endif