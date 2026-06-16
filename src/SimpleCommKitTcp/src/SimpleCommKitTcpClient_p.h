#pragma once

#include "SimpleCommKitTcp.h"
#include "SimpleCommKitExport.h"

#include <hv/TcpClient.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SimpleCommKit {

class SimpleCommKitTcpClientPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitTcpClient)
public:
    SimpleCommKitTcpClientPrivate(SimpleCommKitTcpClient* parent);
    ~SimpleCommKitTcpClientPrivate();

    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;

    int send(const std::vector<uint8_t>& data);
    int send(const std::string& data);

    void setConnectTimeout(int timeout_ms);
    void setReconnect(const SimpleCommKitTcpReconnectSetting& setting);
    void disableReconnect();

    bool enableTls(const SimpleCommKitTlsSetting& setting);
    bool enableTls();

    void setCallbackOnConnected(std::function<void()> callback);
    void setCallbackOnDisconnected(std::function<void()> callback);
    void setCallbackOnMessage(std::function<void(const std::vector<uint8_t>&)> callback);
    void setCallbackOnError(std::function<void(ErrorCode)> callback);

    void triggerError(ErrorCode error_code);

private:
    SimpleCommKitTcpClient* q_ptr;
    std::unique_ptr<hv::TcpClient> m_client;

    std::function<void()>                               m_onConnected;
    std::function<void()>                               m_onDisconnected;
    std::function<void(const std::vector<uint8_t>&)>    m_onMessage;
    std::function<void(ErrorCode)>                      m_onError;

    mutable std::mutex m_mutex;
    bool m_wasConnected   = false;
    bool m_tlsEnabled      = false;
    SimpleCommKitTlsSetting m_tlsSetting;
};

} // namespace SimpleCommKit
