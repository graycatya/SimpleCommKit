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
// UDP Client
// ---------------------------------------------------------------------------

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitUdpClient)

class SIMPLECOMMKIT_API SimpleCommKitUdpClient
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitUdpClient)
public:
    SimpleCommKitUdpClient();
    ~SimpleCommKitUdpClient();

    // Open / close the local socket for sending and receiving
    bool open(int localPort = 0, const std::string& localHost = "0.0.0.0");
    void close();
    bool isOpen() const;

    // Data transmission – send to an explicit remote address
    int sendTo(const std::string& host, int port, const std::vector<uint8_t>& data);
    int sendTo(const std::string& host, int port, const std::string& data);

    // Convenience: set a default remote address for send()
    void setRemoteAddress(const std::string& host, int port);
    int send(const std::vector<uint8_t>& data);
    int send(const std::string& data);

    // Configuration
    void setReadTimeout(int timeout_ms);

    // Callbacks
    void setCallback_OnMessage(std::function<void(const std::vector<uint8_t>&)> callback);
    void setCallback_OnError(std::function<void(ErrorCode)> callback);

private:
    std::unique_ptr<SimpleCommKitUdpClientPrivate> d_ptr;
};

// ---------------------------------------------------------------------------
// UDP Server
// ---------------------------------------------------------------------------

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitUdpServer)

class SIMPLECOMMKIT_API SimpleCommKitUdpServer
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitUdpServer)
public:
    SimpleCommKitUdpServer();
    ~SimpleCommKitUdpServer();

    // Lifecycle
    bool start(int port, const std::string& host = "0.0.0.0");
    void stop();
    bool isRunning() const;

    // Data transmission – send back to a specific client address
    int sendTo(const std::string& host, int port, const std::vector<uint8_t>& data);
    int sendTo(const std::string& host, int port, const std::string& data);

    // Broadcast to all clients (sends to 255.255.255.255)
    int broadcast(const std::vector<uint8_t>& data);
    int broadcast(const std::string& data);

    // Server info
    uint32_t port() const;
    std::string host() const;

    // Callbacks – OnMessage includes the sender's address (as "ip:port")
    void setCallback_OnMessage(std::function<void(const std::string& fromHost, int fromPort, const std::vector<uint8_t>&)> callback);
    void setCallback_OnError(std::function<void(ErrorCode)> callback);

private:
    std::unique_ptr<SimpleCommKitUdpServerPrivate> d_ptr;
};

} // namespace SimpleCommKit
