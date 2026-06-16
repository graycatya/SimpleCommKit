#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <memory>

#include <SimpleCommKitMqttClient.h>
#include <SimpleCommKitErrorMap.hpp>
#include <SimpleCommKitVersion.h>

namespace py = pybind11;
using namespace SimpleCommKit;

// ============================================================================
// MqttReconnectSetting struct
// ============================================================================
void wrap_reconnect_setting(py::module& m) {
    py::class_<SimpleCommKitMqttReconnectSetting>(m, "MqttReconnectSetting",
        "Reconnect configuration for MQTT client.")
        .def(py::init<>())
        .def_readwrite("min_delay_ms", &SimpleCommKitMqttReconnectSetting::min_delay_ms)
        .def_readwrite("max_delay_ms", &SimpleCommKitMqttReconnectSetting::max_delay_ms)
        .def_readwrite("delay_policy", &SimpleCommKitMqttReconnectSetting::delay_policy)
        .def_readwrite("max_retry_cnt", &SimpleCommKitMqttReconnectSetting::max_retry_cnt)
        .def("__repr__", [](const SimpleCommKitMqttReconnectSetting& s) {
            return "<MqttReconnectSetting min=" + std::to_string(s.min_delay_ms) +
                   " max=" + std::to_string(s.max_delay_ms) + ">";
        });
}

// ============================================================================
// MqttTlsSetting struct
// ============================================================================
void wrap_tls_setting(py::module& m) {
    py::class_<SimpleCommKitMqttTlsSetting>(m, "MqttTlsSetting",
        "TLS/SSL configuration for MQTT connections.")
        .def(py::init<>())
        .def_readwrite("ca_file", &SimpleCommKitMqttTlsSetting::ca_file)
        .def_readwrite("ca_path", &SimpleCommKitMqttTlsSetting::ca_path)
        .def_readwrite("crt_file", &SimpleCommKitMqttTlsSetting::crt_file)
        .def_readwrite("key_file", &SimpleCommKitMqttTlsSetting::key_file)
        .def_readwrite("verify_peer", &SimpleCommKitMqttTlsSetting::verify_peer)
        .def("__repr__", [](const SimpleCommKitMqttTlsSetting& s) {
            return "<MqttTlsSetting verify=" + std::string(s.verify_peer ? "true" : "false") + ">";
        });
}

// ============================================================================
// MqttWillMessage struct
// ============================================================================
void wrap_will_message(py::module& m) {
    py::class_<SimpleCommKitMqttWillMessage>(m, "MqttWillMessage",
        "MQTT Will (Last Will and Testament) message configuration.")
        .def(py::init<>())
        .def_readwrite("topic", &SimpleCommKitMqttWillMessage::topic)
        .def_readwrite("qos", &SimpleCommKitMqttWillMessage::qos)
        .def_readwrite("retain", &SimpleCommKitMqttWillMessage::retain)
        .def_property("payload",
            [](const SimpleCommKitMqttWillMessage& w) -> py::bytes {
                return py::bytes(reinterpret_cast<const char*>(w.payload.data()), w.payload.size());
            },
            [](SimpleCommKitMqttWillMessage& w, py::bytes data) {
                std::string str = data;
                w.payload.assign(str.begin(), str.end());
            })
        .def("__repr__", [](const SimpleCommKitMqttWillMessage& w) {
            return "<MqttWillMessage topic='" + w.topic + "' qos=" + std::to_string(w.qos) + ">";
        });
}

// ============================================================================
// MqttClient class
// ============================================================================
void wrap_mqtt_client(py::module& m) {
    py::class_<SimpleCommKitMqttClient>(m, "MqttClient",
        "MQTT client for connecting to a broker and publishing/subscribing to topics.")

        .def(py::init<>(), "Create a new MqttClient instance.")

        // --- Connection ---
        .def("connect", &SimpleCommKitMqttClient::connect,
            py::arg("host"), py::arg("port") = 1883,
            "Connect to an MQTT broker. Default port is 1883.")
        .def("connect_ssl", &SimpleCommKitMqttClient::connectSsl,
            py::arg("host"), py::arg("port") = 8883,
            "Connect to an MQTT broker over SSL. Default TLS port is 8883.")
        .def("disconnect", &SimpleCommKitMqttClient::disconnect)
        .def("is_connected", &SimpleCommKitMqttClient::isConnected)

        // --- Identity ---
        .def("set_client_id", &SimpleCommKitMqttClient::setClientId,
            py::arg("id"),
            "Set the MQTT client identifier. Call BEFORE connect().")
        .def("set_auth", &SimpleCommKitMqttClient::setAuth,
            py::arg("username"), py::arg("password"),
            "Set login credentials. Call BEFORE connect().")
        .def("set_will", &SimpleCommKitMqttClient::setWill,
            py::arg("will"),
            "Set the Last Will and Testament message. Call BEFORE connect().")

        // --- Configuration ---
        .def("set_ping_interval", &SimpleCommKitMqttClient::setPingInterval,
            py::arg("seconds"),
            "Set the keep-alive ping interval in seconds.")
        .def("set_connect_timeout", &SimpleCommKitMqttClient::setConnectTimeout,
            py::arg("ms"),
            "Set the connection timeout in milliseconds.")
        .def("set_reconnect", &SimpleCommKitMqttClient::setReconnect,
            py::arg("setting"),
            "Configure automatic reconnection using MqttReconnectSetting.")
        .def("disable_reconnect", &SimpleCommKitMqttClient::disableReconnect)

        // --- TLS ---
        .def("enable_tls", [](SimpleCommKitMqttClient& self, const SimpleCommKitMqttTlsSetting& setting) -> bool {
                return self.enableTls(setting);
            },
            py::arg("setting"),
            "Enable TLS with custom certificates. Call BEFORE connect().")
        .def("enable_tls_default", [](SimpleCommKitMqttClient& self) -> bool {
                return self.enableTls();
            },
            "Enable TLS with default platform certificates.")

        // --- Publish ---
        .def("publish", [](SimpleCommKitMqttClient& self, const std::string& topic,
                            py::bytes data, int qos, bool retain) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.publish(topic, vec, qos, retain);
            },
            py::arg("topic"), py::arg("data"), py::arg("qos") = 0, py::arg("retain") = false,
            "Publish a binary message to a topic. Returns message ID or -1 on error.")
        .def("publish_text", [](SimpleCommKitMqttClient& self, const std::string& topic,
                                 const std::string& data, int qos, bool retain) -> int {
                return self.publish(topic, data, qos, retain);
            },
            py::arg("topic"), py::arg("data"), py::arg("qos") = 0, py::arg("retain") = false,
            "Publish a text message to a topic. Returns message ID or -1 on error.")

        // --- Subscribe / Unsubscribe ---
        .def("subscribe", &SimpleCommKitMqttClient::subscribe,
            py::arg("topic"), py::arg("qos") = 0,
            "Subscribe to a topic with the given QoS.")
        .def("unsubscribe", &SimpleCommKitMqttClient::unsubscribe,
            py::arg("topic"),
            "Unsubscribe from a topic.")

        // --- Callbacks ---
        .def("set_callback_on_connected",
            [](SimpleCommKitMqttClient& self, py::function callback) {
                self.setCallback_OnConnected([callback]() {
                    py::gil_scoped_acquire gil;
                    callback();
                });
            },
            py::arg("callback"), py::keep_alive<1, 2>(),
            "Set callback invoked on successful CONNACK.")
        .def("set_callback_on_disconnected",
            [](SimpleCommKitMqttClient& self, py::function callback) {
                self.setCallback_OnDisconnected([callback]() {
                    py::gil_scoped_acquire gil;
                    callback();
                });
            },
            py::arg("callback"), py::keep_alive<1, 2>(),
            "Set callback invoked on disconnect.")
        .def("set_callback_on_message",
            [](SimpleCommKitMqttClient& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.setCallback_OnMessage(
                    [cb](const std::string& topic, const std::vector<uint8_t>& payload) {
                        py::gil_scoped_acquire gil;
                        (*cb)(topic, py::bytes(reinterpret_cast<const char*>(payload.data()), payload.size()));
                    });
            },
            py::arg("callback"), py::keep_alive<1, 2>(),
            "Set callback invoked when a PUBLISH message arrives. Callback receives (topic, bytes).")
        .def("set_callback_on_error",
            [](SimpleCommKitMqttClient& self, py::function callback) {
                self.setCallback_OnError([callback](ErrorCode error_code) {
                    py::gil_scoped_acquire gil;
                    callback(error_code);
                });
            },
            py::arg("callback"), py::keep_alive<1, 2>(),
            "Set callback for error notifications.");
}

// ============================================================================
// Module entry point
// ============================================================================
PYBIND11_MODULE(_SimpleCommKitPyMqttClient, m) {
    m.doc() = R"pbdoc(
        SimpleCommKitPyMqttClient - Python bindings for SimpleCommKit MQTT

        Provides cross-platform MQTT client API for Python.
    )pbdoc";

    m.def("get_error_description", &SimpleCommKitErrorMap::GetErrorDescription,
          py::arg("error_code"),
          "Get a human-readable description for an error code.");

    m.def("get_version", []() {
        return SIMPLECOMMKIT_VERSION;
    }, "Get the SimpleCommKit library version string.");

    wrap_reconnect_setting(m);
    wrap_tls_setting(m);
    wrap_will_message(m);
    wrap_mqtt_client(m);
}
