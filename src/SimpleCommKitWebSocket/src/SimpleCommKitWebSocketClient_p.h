#pragma once

#include "SimpleCommKitWebSocket.h"
#include "SimpleCommKitExport.h"

#include <hv/WebSocketClient.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SimpleCommKit {

class SimpleCommKitWebSocketClientPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitWebSocketClient)
public:
    SimpleCommKitWebSocketClientPrivate(SimpleCommKitWebSocketClient* parent);
    ~SimpleCommKitWebSocketClientPrivate();

    bool open(const std::string& url);
    void close();
    bool isConnected() const;

    int send(const std::string& data);
    int send(const std::vector<uint8_t>& data);

    void setConnectTimeout(int timeout_ms);
    void setReconnect(const SimpleCommKitWebSocketReconnectSetting& setting);
    void disableReconnect();
    void setPingInterval(int ms);

    bool enableTls(const SimpleCommKitWebSocketTlsSetting& setting);
    bool enableTls();

    void setCallbackOnOpen(std::function<void()> callback);
    void setCallbackOnClose(std::function<void()> callback);
    void setCallbackOnMessage(std::function<void(const std::vector<uint8_t>&)> callback);
    void setCallbackOnError(std::function<void(ErrorCode)> callback);

    void triggerError(ErrorCode error_code);

private:
    SimpleCommKitWebSocketClient* q_ptr;
    std::unique_ptr<hv::WebSocketClient> m_client;

    std::function<void()>                               m_onOpen;
    std::function<void()>                               m_onClose;
    std::function<void(const std::vector<uint8_t>&)>    m_onMessage;
    std::function<void(ErrorCode)>                      m_onError;

    mutable std::mutex m_mutex;
    bool m_wasOpen    = false;
    bool m_tlsEnabled  = false;
    SimpleCommKitWebSocketTlsSetting m_tlsSetting;
};

} // namespace SimpleCommKit
