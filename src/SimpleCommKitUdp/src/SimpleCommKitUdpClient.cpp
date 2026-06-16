#include "SimpleCommKitUdp.h"
#include "src/SimpleCommKitUdpClient_p.h"

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitUdpClient (public pimpl)
// ---------------------------------------------------------------------------

SimpleCommKitUdpClient::SimpleCommKitUdpClient()
    : d_ptr(std::make_unique<SimpleCommKitUdpClientPrivate>(this))
{
}

SimpleCommKitUdpClient::~SimpleCommKitUdpClient() = default;

bool SimpleCommKitUdpClient::open(int localPort, const std::string& localHost)
{
    if (!d_ptr) return false;
    return d_ptr->open(localPort, localHost);
}

void SimpleCommKitUdpClient::close()
{
    if (d_ptr) {
        d_ptr->close();
    }
}

bool SimpleCommKitUdpClient::isOpen() const
{
    if (!d_ptr) return false;
    return d_ptr->isOpen();
}

int SimpleCommKitUdpClient::sendTo(const std::string& host, int port, const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->sendTo(host, port, data);
}

int SimpleCommKitUdpClient::sendTo(const std::string& host, int port, const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->sendTo(host, port, data);
}

void SimpleCommKitUdpClient::setRemoteAddress(const std::string& host, int port)
{
    if (d_ptr) {
        d_ptr->setRemoteAddress(host, port);
    }
}

int SimpleCommKitUdpClient::send(const std::vector<uint8_t>& data)
{
    if (!d_ptr) return -1;
    return d_ptr->send(data);
}

int SimpleCommKitUdpClient::send(const std::string& data)
{
    if (!d_ptr) return -1;
    return d_ptr->send(data);
}

void SimpleCommKitUdpClient::setReadTimeout(int timeout_ms)
{
    if (d_ptr) {
        d_ptr->setReadTimeout(timeout_ms);
    }
}

void SimpleCommKitUdpClient::setCallback_OnMessage(std::function<void(const std::vector<uint8_t>&)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnMessage(std::move(callback));
    }
}

void SimpleCommKitUdpClient::setCallback_OnError(std::function<void(ErrorCode)> callback)
{
    if (d_ptr) {
        d_ptr->setCallbackOnError(std::move(callback));
    }
}

} // namespace SimpleCommKit
