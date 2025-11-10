#ifndef ROUTEMSG_HPP
#define ROUTEMSG_HPP

#include <thread>
class RouteMsg
{
public:
    RouteMsg() = default;
    RouteMsg(const RouteMsg& other) delete;
    RouteMsg& operator=(const RouteMsg& other) delete;
    ~RouteMsg();


    void startProcessRuntimeThread();
private:

    std::thread m_discoverAndrouteThread;
    std::thread m_monitorThread;
}
#endif