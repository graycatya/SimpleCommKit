#pragma once

#include <SimpleCommKitErrorMap.hpp>
#include "SimpleCommKitExport.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// Shared types
// ---------------------------------------------------------------------------

struct SimpleCommKitMqttReconnectSetting {
    uint32_t min_delay_ms  = 1000;
    uint32_t max_delay_ms  = 10000;
    uint32_t delay_policy  = 2;       // 0=fixed 1=linear 2+=exponential
    uint32_t max_retry_cnt = 0;       // 0 = unlimited
};

struct SimpleCommKitMqttTlsSetting {
    std::string ca_file;
    std::string ca_path;
    std::string crt_file;
    std::string key_file;
    bool        verify_peer = false;
};

struct SimpleCommKitMqttWillMessage {
    std::string           topic;
    std::vector<uint8_t>  payload;
    int                   qos    = 0;
    bool                  retain = false;
};

// ---------------------------------------------------------------------------
// MQTT Client
// ---------------------------------------------------------------------------

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitMqttClient)

class SIMPLECOMMKIT_API SimpleCommKitMqttClient
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitMqttClient)
public:
    SimpleCommKitMqttClient();
    ~SimpleCommKitMqttClient();

    // Connection management
    bool connect(const std::string& host, int port = 1883);
    bool connectSsl(const std::string& host, int port = 8883);
    void disconnect();
    bool isConnected() const;

    // Client identity (must be set before connect)
    void setClientId(const std::string& id);
    void setAuth(const std::string& username, const std::string& password);
    void setWill(const SimpleCommKitMqttWillMessage& will);

    // Configuration
    void setPingInterval(int seconds);
    void setConnectTimeout(int ms);
    void setReconnect(const SimpleCommKitMqttReconnectSetting& setting);
    void disableReconnect();

    // TLS / SSL (must be called before connect)
    bool enableTls(const SimpleCommKitMqttTlsSetting& setting);
    bool enableTls();

    // Pub/Sub
    int publish(const std::string& topic, const std::vector<uint8_t>& payload,
                int qos = 0, bool retain = false);
    int publish(const std::string& topic, const std::string& payload,
                int qos = 0, bool retain = false);
    int subscribe(const std::string& topic, int qos = 0);
    int unsubscribe(const std::string& topic);

    // Callbacks
    void setCallback_OnConnected(std::function<void()> callback);
    void setCallback_OnDisconnected(std::function<void()> callback);
    void setCallback_OnMessage(std::function<void(const std::string& topic, const std::vector<uint8_t>& payload)> callback);
    void setCallback_OnError(std::function<void(ErrorCode)> callback);

private:
    std::unique_ptr<SimpleCommKitMqttClientPrivate> d_ptr;
};

} // namespace SimpleCommKit
