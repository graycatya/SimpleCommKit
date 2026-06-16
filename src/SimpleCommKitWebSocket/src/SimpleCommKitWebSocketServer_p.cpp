#include "SimpleCommKitWebSocketServer_p.h"

#include <hv/hssl.h>

#include <cstring>

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitWebSocketServerPrivate
// ---------------------------------------------------------------------------

SimpleCommKitWebSocketServerPrivate::SimpleCommKitWebSocketServerPrivate(SimpleCommKitWebSocketServer* parent)
    : q_ptr(parent)
    , m_server(std::make_unique<hv::WebSocketServer>())
{
    // Set up WebSocket service callbacks
    m_service.onopen = [this](const WebSocketChannelPtr& channel, const HttpRequestPtr& /*req*/) {
        uint32_t id = channel->id();
        {
            std::lock_guard<std::mutex> lock(m_channelsMutex);
            m_channels[id] = channel;
        }
        if (m_onClientConnected) {
            m_onClientConnected(id);
        }
    };

    m_service.onclose = [this](const WebSocketChannelPtr& channel) {
        uint32_t id = channel->id();
        {
            std::lock_guard<std::mutex> lock(m_channelsMutex);
            m_channels.erase(id);
        }
        if (m_onClientDisconnected) {
            m_onClientDisconnected(id);
        }
    };

    m_service.onmessage = [this](const WebSocketChannelPtr& channel, const std::string& msg) {
        if (m_onMessage) {
            auto* ptr = reinterpret_cast<const uint8_t*>(msg.data());
            std::vector<uint8_t> data(ptr, ptr + msg.size());
            m_onMessage(channel->id(), data);
        }
    };

    m_server->registerWebSocketService(&m_service);
}

SimpleCommKitWebSocketServerPrivate::~SimpleCommKitWebSocketServerPrivate()
{
    // Nullify service callbacks before stop to prevent use-after-free
    m_service.onopen    = nullptr;
    m_service.onclose   = nullptr;
    m_service.onmessage = nullptr;

    try {
        if (m_server) {
            m_server->stop();
        }
    } catch (...) {
    }
}

bool SimpleCommKitWebSocketServerPrivate::start(int port, const std::string& host)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketStartError);
        return false;
    }

    if (m_running) {
        return true;
    }

    m_port = port;
    m_host = host;

    m_server->setHost(host.c_str());
    m_server->setPort(port);

    m_server->start();
    m_running = true;
    return true;
}

void SimpleCommKitWebSocketServerPrivate::stop()
{
    if (!m_server || !m_running) return;

    // Nullify service callbacks so stop() won't re-enter via onclose
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_service.onopen    = nullptr;
        m_service.onclose   = nullptr;
        m_service.onmessage = nullptr;
    }

    try {
        m_server->stop();
    } catch (...) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketStopError);
    }
    m_running = false;

    {
        std::lock_guard<std::mutex> lockCh(m_channelsMutex);
        m_channels.clear();
    }
}

bool SimpleCommKitWebSocketServerPrivate::isRunning() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_running;
}

int SimpleCommKitWebSocketServerPrivate::sendTo(uint32_t client_id, const std::string& data)
{
    std::lock_guard<std::mutex> lockCh(m_channelsMutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketNotRunningError);
        return -1;
    }

    auto it = m_channels.find(client_id);
    if (it == m_channels.end()) {
        return -1;
    }

    return it->second->send(data);
}

int SimpleCommKitWebSocketServerPrivate::sendTo(uint32_t client_id, const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lockCh(m_channelsMutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketNotRunningError);
        return -1;
    }

    auto it = m_channels.find(client_id);
    if (it == m_channels.end()) {
        return -1;
    }

    return it->second->send(
        reinterpret_cast<const char*>(data.data()),
        static_cast<int>(data.size()),
        WS_OPCODE_BINARY);
}

int SimpleCommKitWebSocketServerPrivate::broadcast(const std::string& data)
{
    std::lock_guard<std::mutex> lockCh(m_channelsMutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketNotRunningError);
        return -1;
    }

    int total = 0;
    for (auto& [id, channel] : m_channels) {
        if (channel->send(data) >= 0) {
            ++total;
        }
    }
    return total;
}

int SimpleCommKitWebSocketServerPrivate::broadcast(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lockCh(m_channelsMutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketNotRunningError);
        return -1;
    }

    int total = 0;
    for (auto& [id, channel] : m_channels) {
        if (channel->send(
                reinterpret_cast<const char*>(data.data()),
                static_cast<int>(data.size()),
                WS_OPCODE_BINARY) >= 0) {
            ++total;
        }
    }
    return total;
}

size_t SimpleCommKitWebSocketServerPrivate::connectionNum() const
{
    std::lock_guard<std::mutex> lockCh(m_channelsMutex);
    return m_channels.size();
}

uint32_t SimpleCommKitWebSocketServerPrivate::port() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint32_t>(m_port);
}

std::string SimpleCommKitWebSocketServerPrivate::host() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_host;
}

void SimpleCommKitWebSocketServerPrivate::setThreadNum(int num)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_server) {
        m_server->setThreadNum(num);
    }
}

void SimpleCommKitWebSocketServerPrivate::setMaxConnectionNum(uint32_t num)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_server) {
        m_server->setMaxWorkerConnectionNum(num);
    }
}

bool SimpleCommKitWebSocketServerPrivate::enableTls(const SimpleCommKitWebSocketTlsSetting& setting)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server) return false;

    m_tlsSetting = setting;

    hssl_ctx_opt_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.crt_file    = m_tlsSetting.crt_file.empty()    ? nullptr : m_tlsSetting.crt_file.c_str();
    opt.key_file    = m_tlsSetting.key_file.empty()    ? nullptr : m_tlsSetting.key_file.c_str();
    opt.ca_file     = m_tlsSetting.ca_file.empty()     ? nullptr : m_tlsSetting.ca_file.c_str();
    opt.ca_path     = m_tlsSetting.ca_path.empty()     ? nullptr : m_tlsSetting.ca_path.c_str();
    opt.verify_peer = m_tlsSetting.verify_peer ? 1 : 0;

    int ret = m_server->newSslCtx(&opt);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketTlsError);
        return false;
    }

    m_tlsEnabled = true;
    return true;
}

bool SimpleCommKitWebSocketServerPrivate::enableTls()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server) return false;

    hssl_ctx_opt_t opt;
    memset(&opt, 0, sizeof(opt));

    int ret = m_server->newSslCtx(&opt);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketTlsError);
        return false;
    }

    m_tlsEnabled = true;
    return true;
}

void SimpleCommKitWebSocketServerPrivate::setCallbackOnClientConnected(std::function<void(uint32_t client_id)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onClientConnected = std::move(callback);
}

void SimpleCommKitWebSocketServerPrivate::setCallbackOnClientDisconnected(std::function<void(uint32_t client_id)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onClientDisconnected = std::move(callback);
}

void SimpleCommKitWebSocketServerPrivate::setCallbackOnMessage(
    std::function<void(uint32_t client_id, const std::vector<uint8_t>&)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onMessage = std::move(callback);
}

void SimpleCommKitWebSocketServerPrivate::setCallbackOnError(std::function<void(ErrorCode)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onError = std::move(callback);
}

void SimpleCommKitWebSocketServerPrivate::triggerError(ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

} // namespace SimpleCommKit
