#include "SimpleCommKitTcpServer_p.h"

#include <hv/hssl.h>
#include <cstring>

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitTcpServerPrivate
// ---------------------------------------------------------------------------

SimpleCommKitTcpServerPrivate::SimpleCommKitTcpServerPrivate(SimpleCommKitTcpServer* parent)
    : q_ptr(parent)
    , m_server(std::make_unique<hv::TcpServer>())
{
    auto* server = m_server.get();

    // Connection callback – client arrives or leaves
    server->onConnection = [this](const hv::SocketChannelPtr& channel) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (channel->isConnected()) {
            // New client connected
            if (m_onClientConnected) {
                m_onClientConnected(channel->id());
            }
        } else {
            // Client disconnected
            if (m_onClientDisconnected) {
                m_onClientDisconnected(channel->id());
            }
        }
    };

    // Message callback
    server->onMessage = [this](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        if (m_onMessage && buf && buf->size() > 0) {
            auto* ptr = static_cast<const uint8_t*>(buf->data());
            std::vector<uint8_t> data(ptr, ptr + buf->size());
            m_onMessage(channel->id(), data);
        }
    };
}

SimpleCommKitTcpServerPrivate::~SimpleCommKitTcpServerPrivate()
{
    try {
        if (m_server) {
            m_server->stop();
        }
    } catch (...) {
    }
}

bool SimpleCommKitTcpServerPrivate::start(int port, const std::string& host)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server) {
        triggerError(ErrorCodes::SimpleCommKitTcpStartError);
        return false;
    }

    if (m_running) {
        return true; // already running
    }

    m_port = port;
    m_host = host;

    int listenfd = m_server->createsocket(port, host.c_str());
    if (listenfd < 0) {
        triggerError(ErrorCodes::SimpleCommKitTcpStartError);
        return false;
    }

    m_server->start();
    m_running = true;
    return true;
}

void SimpleCommKitTcpServerPrivate::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_server && m_running) {
        try {
            m_server->stop();
        } catch (...) {
            triggerError(ErrorCodes::SimpleCommKitTcpStopError);
        }
        m_running = false;
    }
}

bool SimpleCommKitTcpServerPrivate::isRunning() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_running;
}

int SimpleCommKitTcpServerPrivate::sendTo(uint32_t client_id, const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitTcpNotRunningError);
        return -1;
    }

    auto channel = m_server->getChannelById(client_id);
    if (!channel) {
        return -1;
    }
    return channel->write(data.data(), static_cast<int>(data.size()));
}

int SimpleCommKitTcpServerPrivate::sendTo(uint32_t client_id, const std::string& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitTcpNotRunningError);
        return -1;
    }

    auto channel = m_server->getChannelById(client_id);
    if (!channel) {
        return -1;
    }
    return channel->write(data.data(), static_cast<int>(data.size()));
}

int SimpleCommKitTcpServerPrivate::broadcast(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitTcpNotRunningError);
        return -1;
    }

    int ret = m_server->broadcast(data.data(), static_cast<int>(data.size()));
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitTcpBroadcastError);
    }
    return ret;
}

int SimpleCommKitTcpServerPrivate::broadcast(const std::string& data)
{
    return broadcast(std::vector<uint8_t>(data.begin(), data.end()));
}

size_t SimpleCommKitTcpServerPrivate::connectionNum() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_server) return 0;
    return m_server->connectionNum();
}

uint32_t SimpleCommKitTcpServerPrivate::port() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint32_t>(m_port);
}

std::string SimpleCommKitTcpServerPrivate::host() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_host;
}

void SimpleCommKitTcpServerPrivate::setThreadNum(int num)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_server) {
        m_server->setThreadNum(num);
    }
}

void SimpleCommKitTcpServerPrivate::setMaxConnectionNum(uint32_t num)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_server) {
        m_server->setMaxConnectionNum(num);
    }
}

bool SimpleCommKitTcpServerPrivate::enableTls(const SimpleCommKitTlsSetting& setting)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server) return false;

    // Keep a copy so c_str() remains valid for the shallow copy in withTLS()
    m_tlsSetting = setting;

    hssl_ctx_opt_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.crt_file    = m_tlsSetting.crt_file.empty()    ? nullptr : m_tlsSetting.crt_file.c_str();
    opt.key_file    = m_tlsSetting.key_file.empty()    ? nullptr : m_tlsSetting.key_file.c_str();
    opt.ca_file     = m_tlsSetting.ca_file.empty()     ? nullptr : m_tlsSetting.ca_file.c_str();
    opt.ca_path     = m_tlsSetting.ca_path.empty()     ? nullptr : m_tlsSetting.ca_path.c_str();
    opt.verify_peer = m_tlsSetting.verify_peer ? 1 : 0;

    int ret = m_server->withTLS(&opt);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitTcpTlsError);
        return false;
    }

    m_tlsEnabled = true;
    return true;
}

bool SimpleCommKitTcpServerPrivate::enableTls()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server) return false;

    int ret = m_server->withTLS();
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitTcpTlsError);
        return false;
    }

    m_tlsEnabled = true;
    return true;
}

void SimpleCommKitTcpServerPrivate::setCallbackOnClientConnected(std::function<void(uint32_t client_id)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onClientConnected = std::move(callback);
}

void SimpleCommKitTcpServerPrivate::setCallbackOnClientDisconnected(std::function<void(uint32_t client_id)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onClientDisconnected = std::move(callback);
}

void SimpleCommKitTcpServerPrivate::setCallbackOnMessage(std::function<void(uint32_t client_id, const std::vector<uint8_t>&)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onMessage = std::move(callback);
}

void SimpleCommKitTcpServerPrivate::setCallbackOnError(std::function<void(ErrorCode)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onError = std::move(callback);
}

void SimpleCommKitTcpServerPrivate::triggerError(ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

} // namespace SimpleCommKit
