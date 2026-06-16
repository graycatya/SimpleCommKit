#pragma once

#include "SimpleCommKitUdp.h"
#include "SimpleCommKitExport.h"

#include <hv/UdpClient.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SimpleCommKit {

class SimpleCommKitUdpClientPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitUdpClient)
public:
    SimpleCommKitUdpClientPrivate(SimpleCommKitUdpClient* parent);
    ~SimpleCommKitUdpClientPrivate();

    bool open(int localPort, const std::string& localHost);
    void close();
    bool isOpen() const;

    int sendTo(const std::string& host, int port, const std::vector<uint8_t>& data);
    int sendTo(const std::string& host, int port, const std::string& data);

    void setRemoteAddress(const std::string& host, int port);
    int send(const std::vector<uint8_t>& data);
    int send(const std::string& data);

    void setReadTimeout(int timeout_ms);

    void setCallbackOnMessage(std::function<void(const std::vector<uint8_t>&)> callback);
    void setCallbackOnError(std::function<void(ErrorCode)> callback);

    void triggerError(ErrorCode error_code);

private:
    SimpleCommKitUdpClient* q_ptr;
    std::unique_ptr<hv::UdpClient> m_client;

    std::string m_remoteHost;
    int         m_remotePort = -1;
    int         m_localPort  = 0;
    std::string m_localHost;

    std::function<void(const std::vector<uint8_t>&)> m_onMessage;
    std::function<void(ErrorCode)>                    m_onError;

    mutable std::mutex m_mutex;
    bool m_isOpen  = false;
};

} // namespace SimpleCommKit
