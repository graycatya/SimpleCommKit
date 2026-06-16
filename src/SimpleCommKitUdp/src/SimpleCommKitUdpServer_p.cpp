#include "SimpleCommKitUdpServer_p.h"

#include <hv/hloop.h>
#include <hv/hsocket.h>

#include <cstring>

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// Helper: extract host (ip) and port from a "ip:port" string (SocketChannel::peeraddr())
// ---------------------------------------------------------------------------
static void splitHostPort(const std::string& addrStr, std::string& outHost, int& outPort) {
    auto pos = addrStr.rfind(':');
    if (pos != std::string::npos) {
        outHost = addrStr.substr(0, pos);
        outPort = std::stoi(addrStr.substr(pos + 1));
    } else {
        outHost = addrStr;
        outPort = 0;
    }
}

// ---------------------------------------------------------------------------
// SimpleCommKitUdpServerPrivate
// ---------------------------------------------------------------------------

SimpleCommKitUdpServerPrivate::SimpleCommKitUdpServerPrivate(SimpleCommKitUdpServer* parent)
    : q_ptr(parent)
    , m_server(std::make_unique<hv::UdpServer>())
{
    auto* server = m_server.get();

    // Message callback — extract sender address from the channel
    server->onMessage = [this](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        if (m_onMessage && buf && buf->size() > 0) {
            auto* ptr = static_cast<const uint8_t*>(buf->data());
            std::vector<uint8_t> data(ptr, ptr + buf->size());

            std::string addrStr = channel->peeraddr();
            std::string fromHost;
            int fromPort = 0;
            splitHostPort(addrStr, fromHost, fromPort);

            m_onMessage(fromHost, fromPort, data);
        }
    };
}

SimpleCommKitUdpServerPrivate::~SimpleCommKitUdpServerPrivate()
{
    try {
        if (m_server) {
            m_server->stop();
        }
    } catch (...) {
    }
}

bool SimpleCommKitUdpServerPrivate::start(int port, const std::string& host)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server) {
        triggerError(ErrorCodes::SimpleCommKitUdpStartError);
        return false;
    }

    if (m_running) {
        return true; // already running
    }

    m_port = port;
    m_host = host;

    int sockfd = m_server->createsocket(port, host.c_str());
    if (sockfd < 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpStartError);
        return false;
    }

    // Enable broadcast by default for the server socket
    if (m_server->channel) {
        int fd = m_server->channel->fd();
        int on = 1;
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof(on));
    }

    m_server->start();
    m_running = true;
    return true;
}

void SimpleCommKitUdpServerPrivate::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_server && m_running) {
        try {
            m_server->stop();
        } catch (...) {
            triggerError(ErrorCodes::SimpleCommKitUdpStopError);
        }
        m_running = false;
    }
}

bool SimpleCommKitUdpServerPrivate::isRunning() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_running;
}

int SimpleCommKitUdpServerPrivate::sendTo(const std::string& host, int port, const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitUdpNotRunningError);
        return -1;
    }

    sockaddr_u addr;
    memset(&addr, 0, sizeof(addr));
    if (sockaddr_set_ipport(&addr, host.c_str(), port) != 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
        return -1;
    }

    int ret = m_server->sendto(data.data(), static_cast<int>(data.size()), &addr.sa);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
    }
    return ret;
}

int SimpleCommKitUdpServerPrivate::sendTo(const std::string& host, int port, const std::string& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitUdpNotRunningError);
        return -1;
    }

    sockaddr_u addr;
    memset(&addr, 0, sizeof(addr));
    if (sockaddr_set_ipport(&addr, host.c_str(), port) != 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
        return -1;
    }

    int ret = m_server->sendto(data.data(), static_cast<int>(data.size()), &addr.sa);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpSendError);
    }
    return ret;
}

int SimpleCommKitUdpServerPrivate::broadcast(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_server || !m_running) {
        triggerError(ErrorCodes::SimpleCommKitUdpNotRunningError);
        return -1;
    }

    sockaddr_u addr;
    memset(&addr, 0, sizeof(addr));
    if (sockaddr_set_ipport(&addr, "255.255.255.255", m_port) != 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpBroadcastError);
        return -1;
    }

    int ret = m_server->sendto(data.data(), static_cast<int>(data.size()), &addr.sa);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUdpBroadcastError);
    }
    return ret;
}

int SimpleCommKitUdpServerPrivate::broadcast(const std::string& data)
{
    return broadcast(std::vector<uint8_t>(data.begin(), data.end()));
}

uint32_t SimpleCommKitUdpServerPrivate::port() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint32_t>(m_port);
}

std::string SimpleCommKitUdpServerPrivate::host() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_host;
}

void SimpleCommKitUdpServerPrivate::setCallbackOnMessage(
    std::function<void(const std::string& fromHost, int fromPort, const std::vector<uint8_t>&)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onMessage = std::move(callback);
}

void SimpleCommKitUdpServerPrivate::setCallbackOnError(std::function<void(ErrorCode)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onError = std::move(callback);
}

void SimpleCommKitUdpServerPrivate::triggerError(ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

} // namespace SimpleCommKit
