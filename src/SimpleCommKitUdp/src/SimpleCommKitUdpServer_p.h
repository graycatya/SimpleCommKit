#pragma once

#include "SimpleCommKitUdp.h"
#include "SimpleCommKitExport.h"

#include <hv/UdpServer.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SimpleCommKit {

class SimpleCommKitUdpServerPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitUdpServer)
public:
    SimpleCommKitUdpServerPrivate(SimpleCommKitUdpServer* parent);
    ~SimpleCommKitUdpServerPrivate();

    bool start(int port, const std::string& host);
    void stop();
    bool isRunning() const;

    int sendTo(const std::string& host, int port, const std::vector<uint8_t>& data);
    int sendTo(const std::string& host, int port, const std::string& data);

    int broadcast(const std::vector<uint8_t>& data);
    int broadcast(const std::string& data);

    uint32_t port() const;
    std::string host() const;

    void setCallbackOnMessage(std::function<void(const std::string& fromHost, int fromPort, const std::vector<uint8_t>&)> callback);
    void setCallbackOnError(std::function<void(ErrorCode)> callback);

    void triggerError(ErrorCode error_code);

private:
    SimpleCommKitUdpServer* q_ptr;
    std::unique_ptr<hv::UdpServer> m_server;

    int         m_port = 0;
    std::string m_host;

    std::function<void(const std::string& fromHost, int fromPort, const std::vector<uint8_t>&)> m_onMessage;
    std::function<void(ErrorCode)> m_onError;

    mutable std::mutex m_mutex;
    bool m_running = false;
};

} // namespace SimpleCommKit
