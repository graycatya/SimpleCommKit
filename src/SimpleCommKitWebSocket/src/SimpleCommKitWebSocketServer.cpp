#include "SimpleCommKitWebSocket.h"
#include "src/SimpleCommKitWebSocketServer_p.h"

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitWebSocketServer (public pimpl)
// ---------------------------------------------------------------------------

SimpleCommKitWebSocketServer::SimpleCommKitWebSocketServer()
    : d_ptr(std::make_unique<SimpleCommKitWebSocketServerPrivate>(this))
{
}

SimpleCommKitWebSocketServer::~SimpleCommKitWebSocketServer() = default;

bool SimpleCommKitWebSocketServer::start(int port, const std::string& host)
{
    if (!d_ptr) return false;
    return d_ptr->start(port, host);
}

void SimpleCommKitWebSocketServer::stop()
{
    if (d_ptr) {
        d_ptr->stop();
    }
}

bool SimpleCommKitWebSocketServer::isRunning() const
{
    if (!d_ptr) return false;
    return d_ptr->isRunning();
}

int SimpleCommKitWebSocketServer::sendTo(uint32_t client_id, const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->sendTo(client_id, data);
}

int SimpleCommKitWebSocketServer::sendTo(uint32_t client_id, const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->sendTo(client_id, data);
}

int SimpleCommKitWebSocketServer::broadcast(const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->broadcast(data);
}

int SimpleCommKitWebSocketServer::broadcast(const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->broadcast(data);
}

size_t SimpleCommKitWebSocketServer::connectionNum() const
{
    if (!d_ptr) return 0;
    return d_ptr->connectionNum();
}

uint32_t SimpleCommKitWebSocketServer::port() const
{
    if (!d_ptr) return 0;
    return d_ptr->port();
}

std::string SimpleCommKitWebSocketServer::host() const
{
    if (!d_ptr) return "";
    return d_ptr->host();
}

void SimpleCommKitWebSocketServer::setThreadNum(int num)
{
    if (d_ptr) {
        d_ptr->setThreadNum(num);
    }
}

void SimpleCommKitWebSocketServer::setMaxConnectionNum(uint32_t num)
{
    if (d_ptr) {
        d_ptr->setMaxConnectionNum(num);
    }
}

bool SimpleCommKitWebSocketServer::enableTls(const SimpleCommKitWebSocketTlsSetting& setting)
{
    if (!d_ptr) return false;
    return d_ptr->enableTls(setting);
}

bool SimpleCommKitWebSocketServer::enableTls()
{
    if (!d_ptr) return false;
    return d_ptr->enableTls();
}

void SimpleCommKitWebSocketServer::setCallback_OnClientConnected(std::function<void(uint32_t client_id)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnClientConnected(std::move(callback));
    }
}

void SimpleCommKitWebSocketServer::setCallback_OnClientDisconnected(std::function<void(uint32_t client_id)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnClientDisconnected(std::move(callback));
    }
}

void SimpleCommKitWebSocketServer::setCallback_OnMessage(
    std::function<void(uint32_t client_id, const std::vector<uint8_t>&)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnMessage(std::move(callback));
    }
}

void SimpleCommKitWebSocketServer::setCallback_OnError(std::function<void(ErrorCode)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnError(std::move(callback));
    }
}

} // namespace SimpleCommKit
