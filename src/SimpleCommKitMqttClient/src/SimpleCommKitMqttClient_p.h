#pragma once

#include "SimpleCommKitMqttClient.h"
#include "SimpleCommKitExport.h"

#include <hv/mqtt_client.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace SimpleCommKit {

class SimpleCommKitMqttClientPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitMqttClient)
public:
    SimpleCommKitMqttClientPrivate(SimpleCommKitMqttClient* parent);
    ~SimpleCommKitMqttClientPrivate();

    bool connect(const std::string& host, int port = 1883);
    bool connectSsl(const std::string& host, int port = 8883);
    void disconnect();
    bool isConnected() const;

    void setClientId(const std::string& id);
    void setAuth(const std::string& username, const std::string& password);
    void setWill(const SimpleCommKitMqttWillMessage& will);

    void setPingInterval(int seconds);
    void setConnectTimeout(int ms);
    void setReconnect(const SimpleCommKitMqttReconnectSetting& setting);
    void disableReconnect();

    bool enableTls(const SimpleCommKitMqttTlsSetting& setting);
    bool enableTls();

    int publish(const std::string& topic, const std::vector<uint8_t>& payload,
                int qos = 0, bool retain = false);
    int publish(const std::string& topic, const std::string& payload,
                int qos = 0, bool retain = false);
    int subscribe(const std::string& topic, int qos = 0);
    int unsubscribe(const std::string& topic);

    void setCallbackOnConnected(std::function<void()> callback);
    void setCallbackOnDisconnected(std::function<void()> callback);
    void setCallbackOnMessage(std::function<void(const std::string& topic, const std::vector<uint8_t>& payload)> callback);
    void setCallbackOnError(std::function<void(ErrorCode)> callback);

    void triggerError(ErrorCode error_code);

private:
    SimpleCommKitMqttClient* q_ptr;
    std::shared_ptr<hv::MqttClient> m_client;

    std::function<void()>                                                     m_onConnected;
    std::function<void()>                                                     m_onDisconnected;
    std::function<void(const std::string&, const std::vector<uint8_t>&)>      m_onMessage;
    std::function<void(ErrorCode)>                                            m_onError;

    mutable std::mutex m_mutex;
    std::atomic<bool>  m_running    {false};
    std::thread        m_eventThread;
    bool               m_tlsEnabled   = false;
    std::string        m_clientId;
    std::string        m_username;
    std::string        m_password;
    SimpleCommKitMqttWillMessage m_will;
    SimpleCommKitMqttTlsSetting  m_tlsSetting;
    reconn_setting_t   m_reconnSetting;
    bool               m_reconnEnabled = false;
    int                m_connectTimeoutMs = 5000;
    int                m_pingIntervalSec  = 60;
};

} // namespace SimpleCommKit
