#include "SimpleCommKitUdpClient_p.h"

#include <hv/hloop.h>
#include <hv/hsocket.h>

#include <cstring>

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitUdpClientPrivate
// ---------------------------------------------------------------------------

SimpleCommKitUdpClientPrivate::SimpleCommKitUdpClientPrivate(SimpleCommKitUdpClient* parent)
    : q_ptr(parent)
    , m_client(std::make_unique<hv::UdpClient>())
{
    auto* client = m_client.get();

    // Wrapped callbacks that translate libhv events into SimpleCommKit style
    client->onMessage = [this](const hv::SocketChannelPtr& /*channel*/, hv::Buffer* buf) {
        if (m_onMessage && buf && buf->size() > 0) {
            auto* ptr = static_cast<const uint8_t*>(buf->data());
            std::vector<uint8_t> data(ptr, ptr + buf->size());
            m_onMessage(data);
        }
    };
}

SimpleCommKitUdpClientPrivate::~SimpleCommKitUdpClientPrivate()
{
    try {
        if (m_client) {
            m_client->stop();
        }
    } catch (...) {
    }
}

bool SimpleCommKitUdpClientPrivate::open(int localPort, const std::string& localHost)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client) {
        triggerError(ErrorCodes::SimpleCommKitUdpOpenError);
        return false;
    }

    if (m_isOpen) {
        return true; // already open
    }

    // createsocket sets up the IO handle with a default remote (dummy here)
    int sockfd = m_client->createsocket(0, "0.0.0.0");
    if (sockfd < 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpOpenError);
        return false;
    }

    // Bind locally if a specific port/host is requested (for receiving data)
    if (localPort != 0 || localHost != "0.0.0.0") {
        int ret = m_client->bind(localPort, localHost.c_str());
        if (ret != 0) {
            triggerError(ErrorCodes::SimpleCommKitUdpOpenError);
            return false;
        }
    }

    m_localPort = localPort;
    m_localHost = localHost;

    // Start the event loop and begin receiving
    m_client->start();

    m_isOpen = true;
    return true;
}

void SimpleCommKitUdpClientPrivate::close()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_client && m_isOpen) {
        try {
            m_client->stop();
        } catch (...) {
            triggerError(ErrorCodes::SimpleCommKitUdpCloseError);
        }
        m_isOpen = false;
    }
}

bool SimpleCommKitUdpClientPrivate::isOpen() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isOpen;
}

int SimpleCommKitUdpClientPrivate::sendTo(const std::string& host, int port, const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_isOpen) {
        triggerError(ErrorCodes::SimpleCommKitUdpNotOpenError);
        return -1;
    }

    sockaddr_u addr;
    memset(&addr, 0, sizeof(addr));
    if (sockaddr_set_ipport(&addr, host.c_str(), port) != 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
        return -1;
    }

    int ret = m_client->sendto(data.data(), static_cast<int>(data.size()), &addr.sa);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
    }
    return ret;
}

int SimpleCommKitUdpClientPrivate::sendTo(const std::string& host, int port, const std::string& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_isOpen) {
        triggerError(ErrorCodes::SimpleCommKitUdpNotOpenError);
        return -1;
    }

    sockaddr_u addr;
    memset(&addr, 0, sizeof(addr));
    if (sockaddr_set_ipport(&addr, host.c_str(), port) != 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
        return -1;
    }

    int ret = m_client->sendto(data.data(), static_cast<int>(data.size()), &addr.sa);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
    }
    return ret;
}

void SimpleCommKitUdpClientPrivate::setRemoteAddress(const std::string& host, int port)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_remoteHost = host;
    m_remotePort = port;
}

int SimpleCommKitUdpClientPrivate::send(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_isOpen) {
        triggerError(ErrorCodes::SimpleCommKitUdpNotOpenError);
        return -1;
    }

    if (m_remotePort < 0 || m_remoteHost.empty()) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
        return -1;
    }

    sockaddr_u addr;
    memset(&addr, 0, sizeof(addr));
    if (sockaddr_set_ipport(&addr, m_remoteHost.c_str(), m_remotePort) != 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
        return -1;
    }

    int ret = m_client->sendto(data.data(), static_cast<int>(data.size()), &addr.sa);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
    }
    return ret;
}

int SimpleCommKitUdpClientPrivate::send(const std::string& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || !m_isOpen) {
        triggerError(ErrorCodes::SimpleCommKitUdpNotOpenError);
        return -1;
    }

    if (m_remotePort < 0 || m_remoteHost.empty()) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
        return -1;
    }

    sockaddr_u addr;
    memset(&addr, 0, sizeof(addr));
    if (sockaddr_set_ipport(&addr, m_remoteHost.c_str(), m_remotePort) != 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
        return -1;
    }

    int ret = m_client->sendto(data.data(), static_cast<int>(data.size()), &addr.sa);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
    }
    return ret;
}

void SimpleCommKitUdpClientPrivate::setReadTimeout(int timeout_ms)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_client && m_client->channel) {
        m_client->channel->setReadTimeout(timeout_ms);
    }
}

void SimpleCommKitUdpClientPrivate::setCallbackOnMessage(std::function<void(const std::vector<uint8_t>&)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onMessage = std::move(callback);
}

void SimpleCommKitUdpClientPrivate::setCallbackOnError(std::function<void(ErrorCode)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onError = std::move(callback);
}

void SimpleCommKitUdpClientPrivate::triggerError(ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

} // namespace SimpleCommKit
