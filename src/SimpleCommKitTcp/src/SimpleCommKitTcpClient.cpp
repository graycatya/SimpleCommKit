#include "SimpleCommKitTcp.h"
#include "src/SimpleCommKitTcpClient_p.h"

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitTcpClient (public pimpl)
// ---------------------------------------------------------------------------

SimpleCommKitTcpClient::SimpleCommKitTcpClient()
    : d_ptr(std::make_unique<SimpleCommKitTcpClientPrivate>(this))
{
}

SimpleCommKitTcpClient::~SimpleCommKitTcpClient() = default;

bool SimpleCommKitTcpClient::connect(const std::string& host, int port)
{
    if (!d_ptr) return false;
    return d_ptr->connect(host, port);
}

void SimpleCommKitTcpClient::disconnect()
{
    if (d_ptr) {
        d_ptr->disconnect();
    }
}

bool SimpleCommKitTcpClient::isConnected() const
{
    if (!d_ptr) return false;
    return d_ptr->isConnected();
}

int SimpleCommKitTcpClient::send(const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->send(data);
}

int SimpleCommKitTcpClient::send(const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->send(data);
}

void SimpleCommKitTcpClient::setConnectTimeout(int timeout_ms)
{
    if (d_ptr) {
        d_ptr->setConnectTimeout(timeout_ms);
    }
}

void SimpleCommKitTcpClient::setReconnect(const SimpleCommKitTcpReconnectSetting& setting)
{
    if (d_ptr) {
        d_ptr->setReconnect(setting);
    }
}

void SimpleCommKitTcpClient::disableReconnect()
{
    if (d_ptr) {
        d_ptr->disableReconnect();
    }
}

bool SimpleCommKitTcpClient::enableTls(const SimpleCommKitTlsSetting& setting)
{
    if (!d_ptr) return false;
    return d_ptr->enableTls(setting);
}

bool SimpleCommKitTcpClient::enableTls()
{
    if (!d_ptr) return false;
    return d_ptr->enableTls();
}

void SimpleCommKitTcpClient::setCallback_OnConnected(std::function<void()> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnConnected(std::move(callback));
    }
}

void SimpleCommKitTcpClient::setCallback_OnDisconnected(std::function<void()> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnDisconnected(std::move(callback));
    }
}

void SimpleCommKitTcpClient::setCallback_OnMessage(std::function<void(const std::vector<uint8_t>&)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnMessage(std::move(callback));
    }
}

void SimpleCommKitTcpClient::setCallback_OnError(std::function<void(ErrorCode)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnError(std::move(callback));
    }
}

} // namespace SimpleCommKit
