#include "SimpleCommKitMqttClient.h"
#include "src/SimpleCommKitMqttClient_p.h"

namespace SimpleCommKit {

// ---------------------------------------------------------------------------
// SimpleCommKitMqttClient (public pimpl)
// ---------------------------------------------------------------------------

SimpleCommKitMqttClient::SimpleCommKitMqttClient()
    : d_ptr(std::make_unique<SimpleCommKitMqttClientPrivate>(this))
{
}

SimpleCommKitMqttClient::~SimpleCommKitMqttClient() = default;

bool SimpleCommKitMqttClient::connect(const std::string& host, int port)
{
    if (!d_ptr) return false;
    return d_ptr->connect(host, port);
}

bool SimpleCommKitMqttClient::connectSsl(const std::string& host, int port)
{
    if (!d_ptr) return false;
    return d_ptr->connectSsl(host, port);
}

void SimpleCommKitMqttClient::disconnect()
{
    if (d_ptr) {
        d_ptr->disconnect();
    }
}

bool SimpleCommKitMqttClient::isConnected() const
{
    if (!d_ptr) return false;
    return d_ptr->isConnected();
}

void SimpleCommKitMqttClient::setClientId(const std::string& id)
{
    if (d_ptr) d_ptr->setClientId(id);
}

void SimpleCommKitMqttClient::setAuth(const std::string& username, const std::string& password)
{
    if (d_ptr) d_ptr->setAuth(username, password);
}

void SimpleCommKitMqttClient::setWill(const SimpleCommKitMqttWillMessage& will)
{
    if (d_ptr) d_ptr->setWill(will);
}

void SimpleCommKitMqttClient::setPingInterval(int seconds)
{
    if (d_ptr) d_ptr->setPingInterval(seconds);
}

void SimpleCommKitMqttClient::setConnectTimeout(int ms)
{
    if (d_ptr) d_ptr->setConnectTimeout(ms);
}

void SimpleCommKitMqttClient::setReconnect(const SimpleCommKitMqttReconnectSetting& setting)
{
    if (d_ptr) d_ptr->setReconnect(setting);
}

void SimpleCommKitMqttClient::disableReconnect()
{
    if (d_ptr) d_ptr->disableReconnect();
}

bool SimpleCommKitMqttClient::enableTls(const SimpleCommKitMqttTlsSetting& setting)
{
    if (!d_ptr) return false;
    return d_ptr->enableTls(setting);
}

bool SimpleCommKitMqttClient::enableTls()
{
    if (!d_ptr) return false;
    return d_ptr->enableTls();
}

int SimpleCommKitMqttClient::publish(const std::string& topic, const std::vector<uint8_t>& payload,
                                      int qos, bool retain)
{
    if (!d_ptr) return -1;
    return d_ptr->publish(topic, payload, qos, retain);
}

int SimpleCommKitMqttClient::publish(const std::string& topic, const std::string& payload,
                                      int qos, bool retain)
{
    if (!d_ptr) return -1;
    return d_ptr->publish(topic, payload, qos, retain);
}

int SimpleCommKitMqttClient::subscribe(const std::string& topic, int qos)
{
    if (!d_ptr) return -1;
    return d_ptr->subscribe(topic, qos);
}

int SimpleCommKitMqttClient::unsubscribe(const std::string& topic)
{
    if (!d_ptr) return -1;
    return d_ptr->unsubscribe(topic);
}

void SimpleCommKitMqttClient::setCallback_OnConnected(std::function<void()> callback)
{
    if (d_ptr) d_ptr->setCallbackOnConnected(std::move(callback));
}

void SimpleCommKitMqttClient::setCallback_OnDisconnected(std::function<void()> callback)
{
    if (d_ptr) d_ptr->setCallbackOnDisconnected(std::move(callback));
}

void SimpleCommKitMqttClient::setCallback_OnMessage(
    std::function<void(const std::string&, const std::vector<uint8_t>&)> callback)
{
    if (d_ptr) d_ptr->setCallbackOnMessage(std::move(callback));
}

void SimpleCommKitMqttClient::setCallback_OnError(std::function<void(ErrorCode)> callback)
{
    if (d_ptr) d_ptr->setCallbackOnError(std::move(callback));
}

} // namespace SimpleCommKit
