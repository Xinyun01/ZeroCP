#include "introspection/introspection_server.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

namespace zero_copy {
namespace introspection {

/**
 * @brief 服务端内部实现
 */
struct IntrospectionServer::Impl {
    std::atomic<IntrospectionState> state{IntrospectionState::STOPPED};
    IntrospectionConfig config;
    
    std::unique_ptr<std::thread> monitoring_thread;
    std::atomic<bool> should_stop{false};
    
    mutable std::mutex metrics_mutex;
    SystemMetrics current_metrics;
    
    mutable std::mutex callbacks_mutex;
    std::map<uint32_t, EventCallback> callbacks;
    std::atomic<uint32_t> next_callback_id{1};
};

IntrospectionServer::IntrospectionServer() 
    : impl_(std::make_unique<Impl>()) {
}

IntrospectionServer::~IntrospectionServer() {
    stop();
}

bool IntrospectionServer::start(const IntrospectionConfig& config) {
    if (impl_->state.load() == IntrospectionState::RUNNING) {
        return false;
    }

    impl_->state.store(IntrospectionState::STARTING);
    impl_->config = config;
    impl_->should_stop.store(false);

    // 启动监控线程
    impl_->monitoring_thread = std::make_unique<std::thread>(&IntrospectionServer::monitoringLoop, this);
    
    impl_->state.store(IntrospectionState::RUNNING);
    return true;
}

void IntrospectionServer::stop() {
    if (impl_->state.load() == IntrospectionState::STOPPED) {
        return;
    }

    impl_->state.store(IntrospectionState::STOPPING);
    impl_->should_stop.store(true);

    if (impl_->monitoring_thread && impl_->monitoring_thread->joinable()) {
        impl_->monitoring_thread->join();
    }

    impl_->state.store(IntrospectionState::STOPPED);
}

IntrospectionState IntrospectionServer::getState() const {
    return impl_->state.load();
}

SystemMetrics IntrospectionServer::getCurrentMetrics() const {
    std::lock_guard<std::mutex> lock(impl_->metrics_mutex);
    return impl_->current_metrics;
}

uint32_t IntrospectionServer::registerCallback(EventCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->callbacks_mutex);
    uint32_t id = impl_->next_callback_id.fetch_add(1);
    impl_->callbacks[id] = callback;
    return id;
}

void IntrospectionServer::unregisterCallback(uint32_t callback_id) {
    std::lock_guard<std::mutex> lock(impl_->callbacks_mutex);
    impl_->callbacks.erase(callback_id);
}

bool IntrospectionServer::updateConfig(const IntrospectionConfig& config) {
    impl_->config = config;
    return true;
}

IntrospectionConfig IntrospectionServer::getConfig() const {
    return impl_->config;
}

SystemMetrics IntrospectionServer::collectOnce() {
    return collectMetrics();
}

void IntrospectionServer::monitoringLoop() {
    while (!impl_->should_stop.load()) {
        try {
            // 收集系统指标
            SystemMetrics metrics = collectMetrics();
            
            // 更新当前指标
            {
                std::lock_guard<std::mutex> lock(impl_->metrics_mutex);
                impl_->current_metrics = metrics;
            }

            // 创建事件并通知回调
            IntrospectionEvent event;
            event.type = IntrospectionEventType::SYSTEM_UPDATE;
            event.metrics = metrics;
            event.timestamp = std::chrono::system_clock::now();
            
            notifyCallbacks(event);

        } catch (const std::exception& e) {
            // 发送错误事件
            IntrospectionEvent error_event;
            error_event.type = IntrospectionEventType::ERROR;
            error_event.error_message = e.what();
            error_event.timestamp = std::chrono::system_clock::now();
            notifyCallbacks(error_event);
        }

        // 等待下次更新
        std::this_thread::sleep_for(std::chrono::milliseconds(impl_->config.update_interval_ms));
    }
}

SystemMetrics IntrospectionServer::collectMetrics() {
    SystemMetrics metrics;
    metrics.timestamp = std::chrono::system_clock::now();

    if (impl_->config.enable_memory_monitoring) {
        collectMemoryInfo(metrics.memory);
    }

    if (impl_->config.enable_process_monitoring) {
        collectProcessInfo(metrics.processes);
    }

    if (impl_->config.enable_connection_monitoring) {
        collectConnectionInfo(metrics.connections);
    }

    if (impl_->config.enable_load_monitoring) {
        collectLoadInfo(metrics.load);
    }

    return metrics;
}

void IntrospectionServer::collectMemoryInfo(MemoryInfo& mem_info) {
    if (!readMemInfo(mem_info)) {
        // 备选方案：使用 sysinfo
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            mem_info.total_memory = si.totalram * si.mem_unit;
            mem_info.free_memory = si.freeram * si.mem_unit;
            mem_info.used_memory = mem_info.total_memory - mem_info.free_memory;
            mem_info.shared_memory = si.sharedram * si.mem_unit;
            mem_info.buffer_memory = si.bufferram * si.mem_unit;
            mem_info.memory_usage_percent = (double)mem_info.used_memory / mem_info.total_memory * 100.0;
        }
    }
}

void IntrospectionServer::collectProcessInfo(std::vector<ProcessInfo>& processes) {
    processes.clear();

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type != DT_DIR) {
            continue;
        }

        char* endptr;
        uint32_t pid = strtoul(entry->d_name, &endptr, 10);
        if (*endptr != '\0') {
            continue;
        }

        ProcessInfo proc_info;
        if (readProcessInfo(pid, proc_info)) {
            processes.push_back(proc_info);
        }
    }

    closedir(proc_dir);

    // 应用过滤器并排序
    applyProcessFilter(processes);
    
    std::sort(processes.begin(), processes.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.memory_usage > b.memory_usage;
              });
}

void IntrospectionServer::collectConnectionInfo(std::vector<ConnectionInfo>& connections) {
    connections.clear();
    readNetworkConnections(connections);
    applyConnectionFilter(connections);
}

void IntrospectionServer::collectLoadInfo(LoadInfo& load_info) {
    readLoadAvg(load_info);
}

void IntrospectionServer::notifyCallbacks(const IntrospectionEvent& event) {
    std::lock_guard<std::mutex> lock(impl_->callbacks_mutex);
    for (const auto& [id, callback] : impl_->callbacks) {
        try {
            callback(event);
        } catch (...) {
            // 忽略回调异常
        }
    }
}

bool IntrospectionServer::readMemInfo(MemoryInfo& mem_info) {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        uint64_t value;
        std::string unit;

        iss >> key >> value >> unit;

        if (key == "MemTotal:") {
            mem_info.total_memory = value * 1024;
        } else if (key == "MemFree:") {
            mem_info.free_memory = value * 1024;
        } else if (key == "MemAvailable:") {
            mem_info.free_memory = value * 1024;
        } else if (key == "Shmem:") {
            mem_info.shared_memory = value * 1024;
        } else if (key == "Buffers:") {
            mem_info.buffer_memory = value * 1024;
        } else if (key == "Cached:") {
            mem_info.cached_memory = value * 1024;
        }
    }

    mem_info.used_memory = mem_info.total_memory - mem_info.free_memory;
    mem_info.memory_usage_percent = (double)mem_info.used_memory / mem_info.total_memory * 100.0;

    return true;
}

bool IntrospectionServer::readLoadAvg(LoadInfo& load_info) {
    std::ifstream file("/proc/loadavg");
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        iss >> load_info.load_1min >> load_info.load_5min >> load_info.load_15min;
    }

    load_info.cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    load_info.cpu_usage_percent = (load_info.load_1min / load_info.cpu_count) * 100.0;

    return true;
}

bool IntrospectionServer::readProcessInfo(uint32_t pid, ProcessInfo& proc_info) {
    proc_info.pid = pid;

    std::string status_path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream status_file(status_path);
    if (!status_file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(status_file, line)) {
        if (line.substr(0, 5) == "Name:") {
            proc_info.name = line.substr(6);
            // 去除前后空格
            proc_info.name.erase(0, proc_info.name.find_first_not_of(" \t\n\r"));
            proc_info.name.erase(proc_info.name.find_last_not_of(" \t\n\r") + 1);
        } else if (line.substr(0, 6) == "State:") {
            proc_info.state = line.substr(7);
            proc_info.state.erase(0, proc_info.state.find_first_not_of(" \t\n\r"));
        } else if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line.substr(7));
            uint64_t value;
            std::string unit;
            iss >> value >> unit;
            proc_info.memory_usage = value * 1024;
        } else if (line.substr(0, 8) == "Threads:") {
            std::istringstream iss(line.substr(9));
            iss >> proc_info.threads_count;
        }
    }

    // 读取命令行
    std::string cmdline_path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream cmdline_file(cmdline_path);
    if (cmdline_file.is_open()) {
        std::getline(cmdline_file, proc_info.command_line, '\0');
    }

    // 读取启动时间
    std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream stat_file(stat_path);
    if (stat_file.is_open()) {
        std::string line;
        if (std::getline(stat_file, line)) {
            std::istringstream iss(line);
            std::string token;
            for (int i = 0; i < 22; ++i) {
                iss >> token;
            }
            proc_info.start_time = std::stoull(token);
        }
    }

    proc_info.cpu_usage = 0.0;

    return !proc_info.name.empty();
}

bool IntrospectionServer::readNetworkConnections(std::vector<ConnectionInfo>& connections) {
    std::ifstream file("/proc/net/tcp");
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::getline(file, line); // 跳过标题行

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (iss >> token) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 4) {
            ConnectionInfo conn;

            // 解析本地地址
            std::string local_addr = tokens[1];
            size_t colon_pos = local_addr.find(':');
            if (colon_pos != std::string::npos) {
                std::string addr_hex = local_addr.substr(0, colon_pos);
                std::string port_hex = local_addr.substr(colon_pos + 1);

                uint32_t addr = std::stoul(addr_hex, nullptr, 16);
                uint16_t port = std::stoul(port_hex, nullptr, 16);

                struct in_addr in_addr;
                in_addr.s_addr = addr;
                conn.local_address = std::string(inet_ntoa(in_addr)) + ":" + std::to_string(port);
            }

            // 解析远程地址
            std::string remote_addr = tokens[2];
            colon_pos = remote_addr.find(':');
            if (colon_pos != std::string::npos) {
                std::string addr_hex = remote_addr.substr(0, colon_pos);
                std::string port_hex = remote_addr.substr(colon_pos + 1);

                uint32_t addr = std::stoul(addr_hex, nullptr, 16);
                uint16_t port = std::stoul(port_hex, nullptr, 16);

                struct in_addr in_addr;
                in_addr.s_addr = addr;
                conn.remote_address = std::string(inet_ntoa(in_addr)) + ":" + std::to_string(port);
            }

            // 解析状态
            std::string state_hex = tokens[3];
            uint32_t state = std::stoul(state_hex, nullptr, 16);
            const char* state_names[] = {
                "UNKNOWN", "ESTABLISHED", "SYN_SENT", "SYN_RECV", "FIN_WAIT1",
                "FIN_WAIT2", "TIME_WAIT", "CLOSE", "CLOSE_WAIT", "LAST_ACK",
                "LISTEN", "CLOSING"
            };
            conn.state = (state < 12) ? state_names[state] : "UNKNOWN";

            conn.protocol = "TCP";
            conn.bytes_sent = 0;
            conn.bytes_received = 0;
            conn.pid = 0;

            connections.push_back(conn);
        }
    }

    return true;
}

void IntrospectionServer::applyProcessFilter(std::vector<ProcessInfo>& processes) {
    if (impl_->config.process_filter.empty()) {
        return;
    }

    auto it = std::remove_if(processes.begin(), processes.end(),
                             [this](const ProcessInfo& proc) {
                                 for (const auto& filter : impl_->config.process_filter) {
                                     if (proc.name.find(filter) != std::string::npos) {
                                         return false;
                                     }
                                 }
                                 return true;
                             });
    processes.erase(it, processes.end());
}

void IntrospectionServer::applyConnectionFilter(std::vector<ConnectionInfo>& connections) {
    if (impl_->config.connection_filter.empty()) {
        return;
    }

    auto it = std::remove_if(connections.begin(), connections.end(),
                             [this](const ConnectionInfo& conn) {
                                 size_t colon_pos = conn.local_address.find_last_of(':');
                                 if (colon_pos == std::string::npos) {
                                     return true;
                                 }
                                 
                                 uint16_t port = std::stoul(conn.local_address.substr(colon_pos + 1));
                                 return std::find(impl_->config.connection_filter.begin(),
                                                impl_->config.connection_filter.end(),
                                                port) == impl_->config.connection_filter.end();
                             });
    connections.erase(it, connections.end());
}

} // namespace introspection
} // namespace zero_copy

