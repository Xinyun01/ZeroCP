#ifndef SERVICE_DESCRIPTION_HPP
#define SERVICE_DESCRIPTION_HPP


#include "zerocp_foundationLib/vocabulary/include/string.hpp"
#include <cstring>

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
    
    // Getter 方法
    const id_string& getService() const noexcept { return m_service; }
    const id_string& getInstance() const noexcept { return m_instance; }
    const id_string& getEvent() const noexcept { return m_event; }
    
    // 比较操作符（用于匹配）
    bool operator==(const ServiceDescription& other) const noexcept
    {
        return std::strcmp(m_service.c_str(), other.m_service.c_str()) == 0 &&
               std::strcmp(m_instance.c_str(), other.m_instance.c_str()) == 0 &&
               std::strcmp(m_event.c_str(), other.m_event.c_str()) == 0;
    }
    
    bool operator!=(const ServiceDescription& other) const noexcept
    {
        return !(*this == other);
    }
    
private:
    id_string m_service;
    id_string m_instance;
    id_string m_event;
};
} // namespace ZeroCP

#endif // SERVICE_DESCRIPTION_HPP