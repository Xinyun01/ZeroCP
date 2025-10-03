#include "introspection/introspection_client.hpp"
#include "introspection/introspection_server.hpp"
#include <mutex>

namespace zero_copy {
namespace introspection {

/**
 * @brief 客户端内部实现
 */
struct IntrospectionClient::Impl {
    std::shared_ptr<IntrospectionServer> server;
    uint32_t callback_id = 0;
    bool is_connected = false;
    mutable std::mutex mutex;
};

IntrospectionClient::IntrospectionClient()
    : impl_(std::make_unique<Impl>()) {
}

IntrospectionClient::~IntrospectionClient() {
    disconnect();
}

bool IntrospectionClient::connectLocal(std::shared_ptr<IntrospectionServer> server) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!server) {
        return false;
    }

    impl_->server = server;
    impl_->is_connected = true;
    return true;
}

void IntrospectionClient::disconnect() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->is_connected && impl_->callback_id != 0) {
        if (impl_->server) {
            impl_->server->unregisterCallback(impl_->callback_id);
        }
        impl_->callback_id = 0;
    }

    impl_->server.reset();
    impl_->is_connected = false;
}

bool IntrospectionClient::isConnected() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->is_connected && impl_->server != nullptr;
}

bool IntrospectionClient::getMetrics(SystemMetrics& metrics) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->is_connected || !impl_->server) {
        return false;
    }

    try {
        metrics = impl_->server->getCurrentMetrics();
        return true;
    } catch (...) {
        return false;
    }
}

bool IntrospectionClient::getMemoryInfo(MemoryInfo& memory) {
    SystemMetrics metrics;
    if (!getMetrics(metrics)) {
        return false;
    }

    memory = metrics.memory;
    return true;
}

bool IntrospectionClient::getProcessList(std::vector<ProcessInfo>& processes) {
    SystemMetrics metrics;
    if (!getMetrics(metrics)) {
        return false;
    }

    processes = metrics.processes;
    return true;
}

bool IntrospectionClient::getConnectionList(std::vector<ConnectionInfo>& connections) {
    SystemMetrics metrics;
    if (!getMetrics(metrics)) {
        return false;
    }

    connections = metrics.connections;
    return true;
}

bool IntrospectionClient::getLoadInfo(LoadInfo& load) {
    SystemMetrics metrics;
    if (!getMetrics(metrics)) {
        return false;
    }

    load = metrics.load;
    return true;
}

bool IntrospectionClient::subscribe(EventCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->is_connected || !impl_->server) {
        return false;
    }

    // 先取消之前的订阅
    if (impl_->callback_id != 0) {
        impl_->server->unregisterCallback(impl_->callback_id);
    }

    // 注册新的回调
    impl_->callback_id = impl_->server->registerCallback(callback);
    return impl_->callback_id != 0;
}

void IntrospectionClient::unsubscribe() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->callback_id != 0 && impl_->server) {
        impl_->server->unregisterCallback(impl_->callback_id);
        impl_->callback_id = 0;
    }
}

bool IntrospectionClient::requestConfigUpdate(const IntrospectionConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->is_connected || !impl_->server) {
        return false;
    }

    return impl_->server->updateConfig(config);
}

bool IntrospectionClient::getConfig(IntrospectionConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->is_connected || !impl_->server) {
        return false;
    }

    config = impl_->server->getConfig();
    return true;
}

bool IntrospectionClient::requestCollectOnce(SystemMetrics& metrics) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->is_connected || !impl_->server) {
        return false;
    }

    try {
        metrics = impl_->server->collectOnce();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace introspection
} // namespace zero_copy

