#include "publisher.hpp"
#include "process_runtime.hpp"
int main()
{
    ZeroCP::ProcessRuntime::initRuntime("RadarData");
    //Publisher<RadarData> publisher("RadarData", "RadarDataInstance", "RadarDataEvent");
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}