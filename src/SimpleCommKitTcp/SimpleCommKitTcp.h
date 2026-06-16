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

struct SimpleCommKitTcpReconnectSetting {
    uint32_t min_delay_ms  = 1000;   // minimum reconnect delay (ms)
    uint32_t max_delay_ms  = 10000;  // maximum reconnect delay (ms)
    uint32_t delay_policy  = 2;      // 0=fixed 1=linear 2+=exponential (base multiplier)
    uint32_t max_retry_cnt = 0;      // 0 = unlimited
};

struct SimpleCommKitTlsSetting {
    std::string crt_file;    // certificate file path (client cert, optional)
    std::string key_file;    // private key file path (optional)
    std::string ca_file;     // CA certificate file (for verifying peer)
    std::string ca_path;     // CA certificates directory
    bool        verify_peer = false; // whether to verify the peer's certificate
};

// ---------------------------------------------------------------------------
// TCP Client
// ---------------------------------------------------------------------------

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitTcpClient)

class SIMPLECOMMKIT_API SimpleCommKitTcpClient
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitTcpClient)
public:
    SimpleCommKitTcpClient();
    ~SimpleCommKitTcpClient();

    // Connection management
    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;

    // Data transmission
    int send(const std::vector<uint8_t>& data);
    int send(const std::string& data);

    // Configuration
    void setConnectTimeout(int timeout_ms);
    void setReconnect(const SimpleCommKitTcpReconnectSetting& setting);
    void disableReconnect();

    // TLS / SSL (must be called before connect())
    bool enableTls(const SimpleCommKitTlsSetting& setting);
    bool enableTls();  // use built-in platform TLS (no custom certs)

    // Callbacks
    void setCallback_OnConnected(std::function<void()> callback);
    void setCallback_OnDisconnected(std::function<void()> callback);
    void setCallback_OnMessage(std::function<void(const std::vector<uint8_t>&)> callback);
    void setCallback_OnError(std::function<void(ErrorCode)> callback);

private:
    std::unique_ptr<SimpleCommKitTcpClientPrivate> d_ptr;
};

// ---------------------------------------------------------------------------
// TCP Server
// ---------------------------------------------------------------------------

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitTcpServer)

class SIMPLECOMMKIT_API SimpleCommKitTcpServer
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitTcpServer)
public:
    SimpleCommKitTcpServer();
    ~SimpleCommKitTcpServer();

    // Lifecycle
    bool start(int port, const std::string& host = "0.0.0.0");
    void stop();
    bool isRunning() const;

    // Data transmission
    int sendTo(uint32_t client_id, const std::vector<uint8_t>& data);
    int sendTo(uint32_t client_id, const std::string& data);
    int broadcast(const std::vector<uint8_t>& data);
    int broadcast(const std::string& data);

    // Connection info
    size_t connectionNum() const;
    uint32_t port() const;
    std::string host() const;

    // Configuration
    void setThreadNum(int num);
    void setMaxConnectionNum(uint32_t num);

    // TLS / SSL (must be called before start())
    bool enableTls(const SimpleCommKitTlsSetting& setting);
    bool enableTls();  // use built-in platform TLS (no custom certs)

    // Callbacks
    void setCallback_OnClientConnected(std::function<void(uint32_t client_id)> callback);
    void setCallback_OnClientDisconnected(std::function<void(uint32_t client_id)> callback);
    void setCallback_OnMessage(std::function<void(uint32_t client_id, const std::vector<uint8_t>&)> callback);
    void setCallback_OnError(std::function<void(ErrorCode)> callback);

private:
    std::unique_ptr<SimpleCommKitTcpServerPrivate> d_ptr;
};

} // namespace SimpleCommKit
