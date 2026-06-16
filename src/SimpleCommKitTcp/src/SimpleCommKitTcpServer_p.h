#pragma once

#include "SimpleCommKitTcp.h"
#include "SimpleCommKitExport.h"

#include <hv/TcpServer.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SimpleCommKit {

class SimpleCommKitTcpServerPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitTcpServer)
public:
    SimpleCommKitTcpServerPrivate(SimpleCommKitTcpServer* parent);
    ~SimpleCommKitTcpServerPrivate();

    bool start(int port, const std::string& host);
    void stop();
    bool isRunning() const;

    int sendTo(uint32_t client_id, const std::vector<uint8_t>& data);
    int sendTo(uint32_t client_id, const std::string& data);
    int broadcast(const std::vector<uint8_t>& data);
    int broadcast(const std::string& data);

    size_t connectionNum() const;
    uint32_t port() const;
    std::string host() const;

    void setThreadNum(int num);
    void setMaxConnectionNum(uint32_t num);

    bool enableTls(const SimpleCommKitTlsSetting& setting);
    bool enableTls();

    void setCallbackOnClientConnected(std::function<void(uint32_t client_id)> callback);
    void setCallbackOnClientDisconnected(std::function<void(uint32_t client_id)> callback);
    void setCallbackOnMessage(std::function<void(uint32_t client_id, const std::vector<uint8_t>&)> callback);
    void setCallbackOnError(std::function<void(ErrorCode)> callback);

    void triggerError(ErrorCode error_code);

private:
    SimpleCommKitTcpServer* q_ptr;
    std::unique_ptr<hv::TcpServer> m_server;

    int m_port = 0;
    std::string m_host;

    std::function<void(uint32_t client_id)>                           m_onClientConnected;
    std::function<void(uint32_t client_id)>                           m_onClientDisconnected;
    std::function<void(uint32_t client_id, const std::vector<uint8_t>&)> m_onMessage;
    std::function<void(ErrorCode)>                                    m_onError;

    mutable std::mutex m_mutex;
    bool m_running    = false;
    bool m_tlsEnabled  = false;
    SimpleCommKitTlsSetting m_tlsSetting;
};

} // namespace SimpleCommKit
