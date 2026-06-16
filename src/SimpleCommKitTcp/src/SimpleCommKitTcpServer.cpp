#include "SimpleCommKitTcp.h"
#include "src/SimpleCommKitTcpServer_p.h"

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitTcpServer (public pimpl)
// ---------------------------------------------------------------------------

SimpleCommKitTcpServer::SimpleCommKitTcpServer()
    : d_ptr(std::make_unique<SimpleCommKitTcpServerPrivate>(this))
{
}

SimpleCommKitTcpServer::~SimpleCommKitTcpServer() = default;

bool SimpleCommKitTcpServer::start(int port, const std::string& host)
{
    if (!d_ptr) return false;
    return d_ptr->start(port, host);
}

void SimpleCommKitTcpServer::stop()
{
    if (d_ptr) {
        d_ptr->stop();
    }
}

bool SimpleCommKitTcpServer::isRunning() const
{
    if (!d_ptr) return false;
    return d_ptr->isRunning();
}

int SimpleCommKitTcpServer::sendTo(uint32_t client_id, const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->sendTo(client_id, data);
}

int SimpleCommKitTcpServer::sendTo(uint32_t client_id, const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->sendTo(client_id, data);
}

int SimpleCommKitTcpServer::broadcast(const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->broadcast(data);
}

int SimpleCommKitTcpServer::broadcast(const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->broadcast(data);
}

size_t SimpleCommKitTcpServer::connectionNum() const
{
    if (!d_ptr) return 0;
    return d_ptr->connectionNum();
}

uint32_t SimpleCommKitTcpServer::port() const
{
    if (!d_ptr) return 0;
    return d_ptr->port();
}

std::string SimpleCommKitTcpServer::host() const
{
    if (!d_ptr) return "";
    return d_ptr->host();
}

void SimpleCommKitTcpServer::setThreadNum(int num)
{
    if (d_ptr) {
        d_ptr->setThreadNum(num);
    }
}

void SimpleCommKitTcpServer::setMaxConnectionNum(uint32_t num)
{
    if (d_ptr) {
        d_ptr->setMaxConnectionNum(num);
    }
}

bool SimpleCommKitTcpServer::enableTls(const SimpleCommKitTlsSetting& setting)
{
    if (!d_ptr) return false;
    return d_ptr->enableTls(setting);
}

bool SimpleCommKitTcpServer::enableTls()
{
    if (!d_ptr) return false;
    return d_ptr->enableTls();
}

void SimpleCommKitTcpServer::setCallback_OnClientConnected(std::function<void(uint32_t client_id)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnClientConnected(std::move(callback));
    }
}

void SimpleCommKitTcpServer::setCallback_OnClientDisconnected(std::function<void(uint32_t client_id)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnClientDisconnected(std::move(callback));
    }
}

void SimpleCommKitTcpServer::setCallback_OnMessage(std::function<void(uint32_t client_id, const std::vector<uint8_t>&)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnMessage(std::move(callback));
    }
}

void SimpleCommKitTcpServer::setCallback_OnError(std::function<void(ErrorCode)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnError(std::move(callback));
    }
}

} // namespace SimpleCommKit
