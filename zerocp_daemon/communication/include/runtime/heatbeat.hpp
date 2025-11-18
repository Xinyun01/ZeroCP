#ifndef ZEROCP_HEATBEAT_HPP
#define ZEROCP_HEATBEAT_HPP

class Heartbeat
{
public:
    Heartbeat() = default;
    Heartbeat(const Heartbeat& other) = delete;
    Heartbeat(Heartbeat&& other) noexcept = delete;
    Heartbeat& operator=(const Heartbeat& other) = delete;
    Heartbeat& operator=(Heartbeat&& other) noexcept = delete;
    ~Heartbeat() noexcept = default;
private:
    void processHeartbeatMessagesThread() noexcept;
    std::thread m_processHeartbeatMessagesThread;
    std::atomic<bool> m_runHeartbeatThread{false};
};
#endif // ZEROCP_HEATBEAT_HPP
