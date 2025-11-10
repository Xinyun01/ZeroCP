#ifndef SERVICE_DESCRIPTION_HPP
#define SERVICE_DESCRIPTION_HPP


#include "zerocp_foundationLib/vocabulary/include/string.hpp"

namespace ZeroCP
{
using id_string = ZeroCP::string<64>;
class ServiceDescription
{
public:
    // 构造函数，初始化服务、实例和事件的名称
    ServiceDescription(id_string service, id_string instance, id_string event)
                    :m_service(service), m_instance(instance), m_event(event){}
    ServiceDescription(const ServiceDescription& other) = default;
    ServiceDescription(ServiceDescription&& other) noexcept = default;
    ServiceDescription& operator=(const ServiceDescription& other) = default;
    ServiceDescription& operator=(ServiceDescription&& other) noexcept = default;
    ~ServiceDescription() = default;
    
private:
    id_string m_service;
    id_string m_instance;
    id_string m_event;
}
} // namespace ZeroCP

#endif // SERVICE_DESCRIPTION_HPP