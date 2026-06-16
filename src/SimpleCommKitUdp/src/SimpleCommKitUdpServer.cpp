#include "SimpleCommKitUdp.h"
#include "src/SimpleCommKitUdpServer_p.h"

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitUdpServer (public pimpl)
// ---------------------------------------------------------------------------

SimpleCommKitUdpServer::SimpleCommKitUdpServer()
    : d_ptr(std::make_unique<SimpleCommKitUdpServerPrivate>(this))
{
}

SimpleCommKitUdpServer::~SimpleCommKitUdpServer() = default;

bool SimpleCommKitUdpServer::start(int port, const std::string& host)
{
    if (!d_ptr) return false;
    return d_ptr->start(port, host);
}

void SimpleCommKitUdpServer::stop()
{
    if (d_ptr) {
        d_ptr->stop();
    }
}

bool SimpleCommKitUdpServer::isRunning() const
{
    if (!d_ptr) return false;
    return d_ptr->isRunning();
}

int SimpleCommKitUdpServer::sendTo(const std::string& host, int port, const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->sendTo(host, port, data);
}

int SimpleCommKitUdpServer::sendTo(const std::string& host, int port, const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->sendTo(host, port, data);
}

int SimpleCommKitUdpServer::broadcast(const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->broadcast(data);
}

int SimpleCommKitUdpServer::broadcast(const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->broadcast(data);
}

uint32_t SimpleCommKitUdpServer::port() const
{
    if (!d_ptr) return 0;
    return d_ptr->port();
}

std::string SimpleCommKitUdpServer::host() const
{
    if (!d_ptr) return "";
    return d_ptr->host();
}

void SimpleCommKitUdpServer::setCallback_OnMessage(
    std::function<void(const std::string& fromHost, int fromPort, const std::vector<uint8_t>&)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnMessage(std::move(callback));
    }
}

void SimpleCommKitUdpServer::setCallback_OnError(std::function<void(ErrorCode)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnError(std::move(callback));
    }
}

} // namespace SimpleCommKit
