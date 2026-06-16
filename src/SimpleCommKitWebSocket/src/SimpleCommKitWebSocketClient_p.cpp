#include "SimpleCommKitWebSocketClient_p.h"

#include <hv/hsocket.h>
#include <hv/hssl.h>

#include <cstring>

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitWebSocketClientPrivate
// ---------------------------------------------------------------------------

SimpleCommKitWebSocketClientPrivate::SimpleCommKitWebSocketClientPrivate(SimpleCommKitWebSocketClient* parent)
    : q_ptr(parent)
    , m_client(std::make_unique<hv::WebSocketClient>())
{
    auto* client = m_client.get();

    client->onopen = [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_wasOpen = true;
        if (m_onOpen) {
            m_onOpen();
        }
    };

    client->onclose = [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_wasOpen) {
            m_wasOpen = false;
            if (m_onClose) {
                m_onClose();
            }
        }
    };

    client->onmessage = [this](const std::string& msg) {
        if (m_onMessage) {
            // Convert string to bytes (supports both text and binary)
            auto* ptr = reinterpret_cast<const uint8_t*>(msg.data());
            std::vector<uint8_t> data(ptr, ptr + msg.size());
            m_onMessage(data);
        }
    };
}

SimpleCommKitWebSocketClientPrivate::~SimpleCommKitWebSocketClientPrivate()
{
    try {
        if (m_client) {
            // Nullify callbacks so close() won't trigger re-entry into member data
            m_client->onopen    = nullptr;
            m_client->onclose   = nullptr;
            m_client->onmessage = nullptr;
            m_client->close();
        }
    } catch (...) {
    }
}

bool SimpleCommKitWebSocketClientPrivate::open(const std::string& url)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketConnectError);
        return false;
    }

    if (m_client->isConnected()) {
        // nullify callbacks so close() won't deadlock on m_mutex via onclose
        m_client->onopen    = nullptr;
        m_client->onclose   = nullptr;
        m_client->onmessage = nullptr;
        m_client->close();
        // restore callbacks after close
        m_client->onopen    = [this]() {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_wasOpen = true;
            if (m_onOpen) m_onOpen();
        };
        m_client->onclose   = [this]() {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (m_wasOpen) { m_wasOpen = false; if (m_onClose) m_onClose(); }
        };
        m_client->onmessage = [this](const std::string& msg) {
            if (m_onMessage) {
                auto* ptr = reinterpret_cast<const uint8_t*>(msg.data());
                m_onMessage(std::vector<uint8_t>(ptr, ptr + msg.size()));
            }
        };
    }

    int ret = m_client->open(url.c_str());
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketConnectError);
        return false;
    }

    return true;
}

void SimpleCommKitWebSocketClientPrivate::close()
{
    if (!m_client) return;

    // Nullify callbacks so m_client->close() won't re-enter m_mutex via onclose
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_client->onopen    = nullptr;
        m_client->onclose   = nullptr;
        m_client->onmessage = nullptr;
    }

    try {
        m_client->close();
    } catch (...) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketDisconnectError);
    }
}

bool SimpleCommKitWebSocketClientPrivate::isConnected() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_client) return false;
    return m_client->isConnected();
}

int SimpleCommKitWebSocketClientPrivate::send(const std::string& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_client->isConnected()) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketNotConnectedError);
        return -1;
    }

    int ret = m_client->send(data);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketSendError);
    }
    return ret;
}

int SimpleCommKitWebSocketClientPrivate::send(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_client->isConnected()) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketNotConnectedError);
        return -1;
    }

    int ret = m_client->send(
        reinterpret_cast<const char*>(data.data()),
        static_cast<int>(data.size()),
        WS_OPCODE_BINARY);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketSendError);
    }
    return ret;
}

void SimpleCommKitWebSocketClientPrivate::setConnectTimeout(int timeout_ms)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_client) {
        m_client->setConnectTimeout(timeout_ms);
    }
}

void SimpleCommKitWebSocketClientPrivate::setReconnect(const SimpleCommKitWebSocketReconnectSetting& setting)
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

void SimpleCommKitWebSocketClientPrivate::disableReconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_client) {
        m_client->setReconnect(nullptr);
    }
}

void SimpleCommKitWebSocketClientPrivate::setPingInterval(int ms)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_client) {
        m_client->setPingInterval(ms);
    }
}

bool SimpleCommKitWebSocketClientPrivate::enableTls(const SimpleCommKitWebSocketTlsSetting& setting)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) return false;

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
        triggerError(ErrorCodes::SimpleCommKitWebSocketTlsError);
        return false;
    }

    m_tlsEnabled = true;
    return true;
}

bool SimpleCommKitWebSocketClientPrivate::enableTls()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) return false;

    int ret = m_client->withTLS();
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitWebSocketTlsError);
        return false;
    }

    m_tlsEnabled = true;
    return true;
}

void SimpleCommKitWebSocketClientPrivate::setCallbackOnOpen(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onOpen = std::move(callback);
}

void SimpleCommKitWebSocketClientPrivate::setCallbackOnClose(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onClose = std::move(callback);
}

void SimpleCommKitWebSocketClientPrivate::setCallbackOnMessage(std::function<void(const std::vector<uint8_t>&)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onMessage = std::move(callback);
}

void SimpleCommKitWebSocketClientPrivate::setCallbackOnError(std::function<void(ErrorCode)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onError = std::move(callback);
}

void SimpleCommKitWebSocketClientPrivate::triggerError(ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

} // namespace SimpleCommKit
