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

struct SimpleCommKitWebSocketReconnectSetting {
    uint32_t min_delay_ms  = 1000;
    uint32_t max_delay_ms  = 10000;
    uint32_t delay_policy  = 2;       // 0=fixed, 1=linear, 2+=exponential
    uint32_t max_retry_cnt = 0;       // 0 = unlimited
};

struct SimpleCommKitWebSocketTlsSetting {
    std::string crt_file;
    std::string key_file;
    std::string ca_file;
    std::string ca_path;
    bool        verify_peer = false;
};

// ---------------------------------------------------------------------------
// WebSocket Client
// ---------------------------------------------------------------------------

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitWebSocketClient)

class SIMPLECOMMKIT_API SimpleCommKitWebSocketClient
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitWebSocketClient)
public:
    SimpleCommKitWebSocketClient();
    ~SimpleCommKitWebSocketClient();

    // Connect to ws://host:port/path or wss://host:port/path
    bool open(const std::string& url);
    void close();
    bool isConnected() const;

    // Data transmission
    int send(const std::string& data);
    int send(const std::vector<uint8_t>& data);

    // Configuration
    void setConnectTimeout(int timeout_ms);
    void setReconnect(const SimpleCommKitWebSocketReconnectSetting& setting);
    void disableReconnect();
    void setPingInterval(int ms);

    // TLS (for explicit cert config; wss:// auto-detects TLS from URL)
    bool enableTls(const SimpleCommKitWebSocketTlsSetting& setting);
    bool enableTls();

    // Callbacks
    void setCallback_OnOpen(std::function<void()> callback);
    void setCallback_OnClose(std::function<void()> callback);
    void setCallback_OnMessage(std::function<void(const std::vector<uint8_t>&)> callback);
    void setCallback_OnError(std::function<void(ErrorCode)> callback);

private:
    std::unique_ptr<SimpleCommKitWebSocketClientPrivate> d_ptr;
};

// ---------------------------------------------------------------------------
// WebSocket Server
// ---------------------------------------------------------------------------

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitWebSocketServer)

class SIMPLECOMMKIT_API SimpleCommKitWebSocketServer
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitWebSocketServer)
public:
    SimpleCommKitWebSocketServer();
    ~SimpleCommKitWebSocketServer();

    // Lifecycle
    bool start(int port, const std::string& host = "0.0.0.0");
    void stop();
    bool isRunning() const;

    // Data transmission
    int sendTo(uint32_t client_id, const std::string& data);
    int sendTo(uint32_t client_id, const std::vector<uint8_t>& data);
    int broadcast(const std::string& data);
    int broadcast(const std::vector<uint8_t>& data);

    // Server info
    size_t connectionNum() const;
    uint32_t port() const;
    std::string host() const;

    // Configuration
    void setThreadNum(int num);
    void setMaxConnectionNum(uint32_t num);

    // TLS (wss://)
    bool enableTls(const SimpleCommKitWebSocketTlsSetting& setting);
    bool enableTls();

    // Callbacks
    void setCallback_OnClientConnected(std::function<void(uint32_t client_id)> callback);
    void setCallback_OnClientDisconnected(std::function<void(uint32_t client_id)> callback);
    void setCallback_OnMessage(std::function<void(uint32_t client_id, const std::vector<uint8_t>&)> callback);
    void setCallback_OnError(std::function<void(ErrorCode)> callback);

private:
    std::unique_ptr<SimpleCommKitWebSocketServerPrivate> d_ptr;
};

} // namespace SimpleCommKit
