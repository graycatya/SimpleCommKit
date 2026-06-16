#include "SimpleCommKitWebSocket.h"
#include "src/SimpleCommKitWebSocketClient_p.h"

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitWebSocketClient (public pimpl)
// ---------------------------------------------------------------------------

SimpleCommKitWebSocketClient::SimpleCommKitWebSocketClient()
    : d_ptr(std::make_unique<SimpleCommKitWebSocketClientPrivate>(this))
{
}

SimpleCommKitWebSocketClient::~SimpleCommKitWebSocketClient() = default;

bool SimpleCommKitWebSocketClient::open(const std::string& url)
{
    if (!d_ptr) return false;
    return d_ptr->open(url);
}

void SimpleCommKitWebSocketClient::close()
{
    if (d_ptr) {
        d_ptr->close();
    }
}

bool SimpleCommKitWebSocketClient::isConnected() const
{
    if (!d_ptr) return false;
    return d_ptr->isConnected();
}

int SimpleCommKitWebSocketClient::send(const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->send(data);
}

int SimpleCommKitWebSocketClient::send(const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->send(data);
}

void SimpleCommKitWebSocketClient::setConnectTimeout(int timeout_ms)
{
    if (d_ptr) {
        d_ptr->setConnectTimeout(timeout_ms);
    }
}

void SimpleCommKitWebSocketClient::setReconnect(const SimpleCommKitWebSocketReconnectSetting& setting)
{
    if (d_ptr) {
        d_ptr->setReconnect(setting);
    }
}

void SimpleCommKitWebSocketClient::disableReconnect()
{
    if (d_ptr) {
        d_ptr->disableReconnect();
    }
}

void SimpleCommKitWebSocketClient::setPingInterval(int ms)
{
    if (d_ptr) {
        d_ptr->setPingInterval(ms);
    }
}

bool SimpleCommKitWebSocketClient::enableTls(const SimpleCommKitWebSocketTlsSetting& setting)
{
    if (!d_ptr) return false;
    return d_ptr->enableTls(setting);
}

bool SimpleCommKitWebSocketClient::enableTls()
{
    if (!d_ptr) return false;
    return d_ptr->enableTls();
}

void SimpleCommKitWebSocketClient::setCallback_OnOpen(std::function<void()> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnOpen(std::move(callback));
    }
}

void SimpleCommKitWebSocketClient::setCallback_OnClose(std::function<void()> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnClose(std::move(callback));
    }
}

void SimpleCommKitWebSocketClient::setCallback_OnMessage(std::function<void(const std::vector<uint8_t>&)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnMessage(std::move(callback));
    }
}

void SimpleCommKitWebSocketClient::setCallback_OnError(std::function<void(ErrorCode)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnError(std::move(callback));
    }
}

} // namespace SimpleCommKit
