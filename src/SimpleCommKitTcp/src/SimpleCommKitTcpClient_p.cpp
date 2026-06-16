#include "SimpleCommKitTcpClient_p.h"

#include <hv/hloop.h>
#include <hv/hsocket.h>
#include <hv/hssl.h>

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitTcpClientPrivate
// ---------------------------------------------------------------------------

SimpleCommKitTcpClientPrivate::SimpleCommKitTcpClientPrivate(SimpleCommKitTcpClient* parent)
    : q_ptr(parent)
    , m_client(std::make_unique<hv::TcpClient>())
{
    auto* client = m_client.get();

    // Wrapped callbacks that translate libhv events into SimpleCommKit style
    client->onConnection = [this](const hv::SocketChannelPtr& channel) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (channel->isConnected()) {
            m_wasConnected = true;
            if (m_onConnected) {
                m_onConnected();
            }
        } else {
            if (m_wasConnected) {
                m_wasConnected = false;
                if (m_onDisconnected) {
                    m_onDisconnected();
                }
            }
        }
    };

    client->onMessage = [this](const hv::SocketChannelPtr& /*channel*/, hv::Buffer* buf) {
        if (m_onMessage && buf && buf->size() > 0) {
            auto* ptr = static_cast<const uint8_t*>(buf->data());
            std::vector<uint8_t> data(ptr, ptr + buf->size());
            m_onMessage(data);
        }
    };
}

SimpleCommKitTcpClientPrivate::~SimpleCommKitTcpClientPrivate()
{
    try {
        if (m_client) {
            m_client->stop();
        }
    } catch (...) {
    }
}

bool SimpleCommKitTcpClientPrivate::connect(const std::string& host, int port)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) {
        triggerError(ErrorCodes::SimpleCommKitTcpConnectError);
        return false;
    }

    // If already connected, disconnect first
    if (m_client->isConnected()) {
        m_client->closesocket();
    }

    int connfd = m_client->createsocket(port, host.c_str());
    if (connfd < 0) {
        triggerError(ErrorCodes::SimpleCommKitTcpConnectError);
        return false;
    }

    m_client->start();
    return true;
}

void SimpleCommKitTcpClientPrivate::disconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_client) {
        try {
            m_client->closesocket();
        } catch (...) {
            triggerError(ErrorCodes::SimpleCommKitTcpDisconnectError);
        }
    }
}

bool SimpleCommKitTcpClientPrivate::isConnected() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) return false;
    return m_client->isConnected();
}

int SimpleCommKitTcpClientPrivate::send(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_client->isConnected()) {
        triggerError(ErrorCodes::SimpleCommKitTcpNotConnectedError);
        return -1;
    }

    int ret = m_client->send(data.data(), static_cast<int>(data.size()));
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitTcpSendError);
    }
    return ret;
}

int SimpleCommKitTcpClientPrivate::send(const std::string& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_client->isConnected()) {
        triggerError(ErrorCodes::SimpleCommKitTcpNotConnectedError);
        return -1;
    }

    int ret = m_client->send(data);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitTcpSendError);
    }
    return ret;
}

void SimpleCommKitTcpClientPrivate::setConnectTimeout(int timeout_ms)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_client) {
        m_client->setConnectTimeout(timeout_ms);
    }
}

void SimpleCommKitTcpClientPrivate::setReconnect(const SimpleCommKitTcpReconnectSetting& setting)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) return;

    reconn_setting_t reconn;
    reconn_setting_init(&reconn);
    reconn.min_delay     = setting.min_delay_ms;
    reconn.max_delay     = setting.max_delay_ms;
    reconn.delay_policy  = setting.delay_policy;
    reconn.max_retry_cnt = setting.max_retry_cnt;

    m_client->setReconnect(&reconn);
}

void SimpleCommKitTcpClientPrivate::disableReconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_client) {
        m_client->setReconnect(nullptr);
    }
}

bool SimpleCommKitTcpClientPrivate::enableTls(const SimpleCommKitTlsSetting& setting)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) return false;

    // Keep a copy so c_str() remains valid for the shallow copy in withTLS()
    m_tlsSetting = setting;

    hssl_ctx_opt_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.crt_file    = m_tlsSetting.crt_file.empty()    ? nullptr : m_tlsSetting.crt_file.c_str();
    opt.key_file    = m_tlsSetting.key_file.empty()    ? nullptr : m_tlsSetting.key_file.c_str();
    opt.ca_file     = m_tlsSetting.ca_file.empty()     ? nullptr : m_tlsSetting.ca_file.c_str();
    opt.ca_path     = m_tlsSetting.ca_path.empty()     ? nullptr : m_tlsSetting.ca_path.c_str();
    opt.verify_peer = m_tlsSetting.verify_peer ? 1 : 0;

    int ret = m_client->withTLS(&opt);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitTcpTlsError);
        return false;
    }

    m_tlsEnabled = true;
    return true;
}

bool SimpleCommKitTcpClientPrivate::enableTls()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) return false;

    int ret = m_client->withTLS();
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitTcpTlsError);
        return false;
    }

    m_tlsEnabled = true;
    return true;
}

void SimpleCommKitTcpClientPrivate::setCallbackOnConnected(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onConnected = std::move(callback);
}

void SimpleCommKitTcpClientPrivate::setCallbackOnDisconnected(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onDisconnected = std::move(callback);
}

void SimpleCommKitTcpClientPrivate::setCallbackOnMessage(std::function<void(const std::vector<uint8_t>&)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onMessage = std::move(callback);
}

void SimpleCommKitTcpClientPrivate::setCallbackOnError(std::function<void(ErrorCode)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onError = std::move(callback);
}

void SimpleCommKitTcpClientPrivate::triggerError(ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

} // namespace SimpleCommKit
