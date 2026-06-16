#include "SimpleCommKitMqttClient_p.h"

#include <hv/hloop.h>
#include <hv/hsocket.h>
#include <hv/hssl.h>

#include <cstring>

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitMqttClientPrivate
// ---------------------------------------------------------------------------

SimpleCommKitMqttClientPrivate::SimpleCommKitMqttClientPrivate(SimpleCommKitMqttClient* parent)
    : q_ptr(parent)
    , m_client(std::make_shared<hv::MqttClient>())
{
    reconn_setting_init(&m_reconnSetting);

    auto* client = m_client.get();

    // Bridge libhv callbacks to SimpleCommKit callbacks
    client->onConnect = [this](hv::MqttClient*) {
        if (m_onConnected) {
            m_onConnected();
        }
    };

    client->onClose = [this](hv::MqttClient*) {
        if (m_onDisconnected) {
            m_onDisconnected();
        }
    };

    client->onMessage = [this](hv::MqttClient*, mqtt_message_t* msg) {
        if (m_onMessage && msg) {
            std::string topic;
            if (msg->topic && msg->topic_len > 0) {
                topic.assign(reinterpret_cast<const char*>(msg->topic), msg->topic_len);
            }

            std::vector<uint8_t> payload;
            if (msg->payload && msg->payload_len > 0) {
                auto* ptr = reinterpret_cast<const uint8_t*>(msg->payload);
                payload.assign(ptr, ptr + msg->payload_len);
            }

            m_onMessage(topic, payload);
        }
    };
}

SimpleCommKitMqttClientPrivate::~SimpleCommKitMqttClientPrivate()
{
    // Stop event thread first
    m_running = false;
    if (m_eventThread.joinable()) {
        m_eventThread.join();
    }
    try {
        if (m_client) {
            m_client->stop();
        }
    } catch (...) {
    }
}

// ---------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------

bool SimpleCommKitMqttClientPrivate::connect(const std::string& host, int port)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) {
        triggerError(ErrorCodes::SimpleCommKitMqttConnectError);
        return false;
    }

    // Apply pre-connect settings
    if (!m_clientId.empty()) {
        m_client->setID(m_clientId.c_str());
    }
    if (!m_username.empty()) {
        m_client->setAuth(m_username.c_str(), m_password.c_str());
    }

    // Will message
    if (!m_will.topic.empty()) {
        mqtt_message_t will_msg;
        memset(&will_msg, 0, sizeof(will_msg));
        will_msg.topic      = m_will.topic.c_str();
        will_msg.topic_len  = m_will.topic.size();
        will_msg.payload    = reinterpret_cast<const char*>(m_will.payload.data());
        will_msg.payload_len = m_will.payload.size();
        will_msg.qos        = m_will.qos;
        will_msg.retain     = m_will.retain ? 1 : 0;
        m_client->setWill(&will_msg);
    }

    m_client->setPingInterval(m_pingIntervalSec);
    m_client->setConnectTimeout(m_connectTimeoutMs);

    if (m_reconnEnabled) {
        m_client->setReconnect(&m_reconnSetting);
    }

    int ssl = m_tlsEnabled ? 1 : 0;
    int ret = m_client->connect(host.c_str(), port, ssl);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitMqttConnectError);
        return false;
    }

    // Run MQTT event loop in a background thread (run() blocks)
    m_running = true;
    m_eventThread = std::thread([this]() {
        m_client->run();
    });
    return true;
}

bool SimpleCommKitMqttClientPrivate::connectSsl(const std::string& host, int port)
{
    m_tlsEnabled = true;
    return connect(host, port);
}

void SimpleCommKitMqttClientPrivate::disconnect()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_client) {
            try {
                m_client->disconnect();
                m_client->stop();
            } catch (...) {
                triggerError(ErrorCodes::SimpleCommKitMqttDisconnectError);
            }
        }
    }

    // Stop event thread (stop() above will cause run() to return)
    m_running = false;
    if (m_eventThread.joinable()) {
        m_eventThread.join();
    }
}

bool SimpleCommKitMqttClientPrivate::isConnected() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) return false;
    return m_client->isConnected();
}

// ---------------------------------------------------------------------------
// Identity / Configuration
// ---------------------------------------------------------------------------

void SimpleCommKitMqttClientPrivate::setClientId(const std::string& id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clientId = id;
}

void SimpleCommKitMqttClientPrivate::setAuth(const std::string& username, const std::string& password)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_username = username;
    m_password = password;
}

void SimpleCommKitMqttClientPrivate::setWill(const SimpleCommKitMqttWillMessage& will)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_will = will;
}

void SimpleCommKitMqttClientPrivate::setPingInterval(int seconds)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pingIntervalSec = seconds;
}

void SimpleCommKitMqttClientPrivate::setConnectTimeout(int ms)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connectTimeoutMs = ms;
}

void SimpleCommKitMqttClientPrivate::setReconnect(const SimpleCommKitMqttReconnectSetting& setting)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_reconnSetting.min_delay     = setting.min_delay_ms;
    m_reconnSetting.max_delay     = setting.max_delay_ms;
    m_reconnSetting.delay_policy  = setting.delay_policy;
    m_reconnSetting.max_retry_cnt = setting.max_retry_cnt;
    m_reconnEnabled = true;
}

void SimpleCommKitMqttClientPrivate::disableReconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_reconnEnabled = false;
}

// ---------------------------------------------------------------------------
// TLS
// ---------------------------------------------------------------------------

bool SimpleCommKitMqttClientPrivate::enableTls(const SimpleCommKitMqttTlsSetting& setting)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) return false;

    m_tlsSetting = setting;

    hssl_ctx_opt_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.ca_file     = m_tlsSetting.ca_file.empty()  ? nullptr : m_tlsSetting.ca_file.c_str();
    opt.ca_path     = m_tlsSetting.ca_path.empty()  ? nullptr : m_tlsSetting.ca_path.c_str();
    opt.crt_file    = m_tlsSetting.crt_file.empty() ? nullptr : m_tlsSetting.crt_file.c_str();
    opt.key_file    = m_tlsSetting.key_file.empty() ? nullptr : m_tlsSetting.key_file.c_str();
    opt.verify_peer = m_tlsSetting.verify_peer ? 1 : 0;

    int ret = m_client->newSslCtx(&opt);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitMqttTlsError);
        return false;
    }

    m_tlsEnabled = true;
    return true;
}

bool SimpleCommKitMqttClientPrivate::enableTls()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tlsEnabled = true;
    return true;
}

// ---------------------------------------------------------------------------
// Pub/Sub
// ---------------------------------------------------------------------------

int SimpleCommKitMqttClientPrivate::publish(const std::string& topic,
                                             const std::vector<uint8_t>& payload,
                                             int qos, bool retain)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_client->isConnected()) {
        triggerError(ErrorCodes::SimpleCommKitMqttNotConnectedError);
        return -1;
    }

    mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.topic       = topic.c_str();
    msg.topic_len   = topic.size();
    msg.payload     = reinterpret_cast<const char*>(payload.data());
    msg.payload_len = payload.size();
    msg.qos         = qos;
    msg.retain      = retain ? 1 : 0;

    int ret = m_client->publish(&msg);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitMqttPublishError);
    }
    return ret;
}

int SimpleCommKitMqttClientPrivate::publish(const std::string& topic,
                                             const std::string& payload,
                                             int qos, bool retain)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_client->isConnected()) {
        triggerError(ErrorCodes::SimpleCommKitMqttNotConnectedError);
        return -1;
    }

    int ret = m_client->publish(topic, payload, qos, retain);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitMqttPublishError);
    }
    return ret;
}

int SimpleCommKitMqttClientPrivate::subscribe(const std::string& topic, int qos)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_client->isConnected()) {
        triggerError(ErrorCodes::SimpleCommKitMqttNotConnectedError);
        return -1;
    }

    int ret = m_client->subscribe(topic.c_str(), qos);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitMqttSubscribeError);
    }
    return ret;
}

int SimpleCommKitMqttClientPrivate::unsubscribe(const std::string& topic)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_client->isConnected()) {
        triggerError(ErrorCodes::SimpleCommKitMqttNotConnectedError);
        return -1;
    }

    int ret = m_client->unsubscribe(topic.c_str());
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitMqttUnsubscribeError);
    }
    return ret;
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

void SimpleCommKitMqttClientPrivate::setCallbackOnConnected(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onConnected = std::move(callback);
}

void SimpleCommKitMqttClientPrivate::setCallbackOnDisconnected(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onDisconnected = std::move(callback);
}

void SimpleCommKitMqttClientPrivate::setCallbackOnMessage(
    std::function<void(const std::string&, const std::vector<uint8_t>&)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onMessage = std::move(callback);
}

void SimpleCommKitMqttClientPrivate::setCallbackOnError(std::function<void(ErrorCode)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onError = std::move(callback);
}

void SimpleCommKitMqttClientPrivate::triggerError(ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

} // namespace SimpleCommKit
