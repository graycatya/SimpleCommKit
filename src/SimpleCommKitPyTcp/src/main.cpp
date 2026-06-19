#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <memory>

#include <SimpleCommKitTcp.h>
#include <SimpleCommKitErrorMap.hpp>
#include <SimpleCommKitVersion.h>

namespace py = pybind11;
using namespace SimpleCommKit;

// ============================================================================
// ReconnectSetting struct
// ============================================================================
void wrap_reconnect_setting(py::module& m) {
    py::class_<SimpleCommKitTcpReconnectSetting>(m, "TcpReconnectSetting",
        "Reconnect configuration for TCP client.")
        .def(py::init<>())
        .def_readwrite("min_delay_ms", &SimpleCommKitTcpReconnectSetting::min_delay_ms,
            "Minimum reconnect delay in milliseconds (default: 1000)")
        .def_readwrite("max_delay_ms", &SimpleCommKitTcpReconnectSetting::max_delay_ms,
            "Maximum reconnect delay in milliseconds (default: 10000)")
        .def_readwrite("delay_policy", &SimpleCommKitTcpReconnectSetting::delay_policy,
            "Delay policy: 0=fixed, 1=linear, 2+=exponential (default: 2)")
        .def_readwrite("max_retry_cnt", &SimpleCommKitTcpReconnectSetting::max_retry_cnt,
            "Maximum retry count (0 = unlimited)")
        .def("__repr__", [](const SimpleCommKitTcpReconnectSetting& s) {
            return "<TcpReconnectSetting min=" + std::to_string(s.min_delay_ms) +
                   " max=" + std::to_string(s.max_delay_ms) +
                   " policy=" + std::to_string(s.delay_policy) +
                   " retries=" + std::to_string(s.max_retry_cnt) + ">";
        });
}

// ============================================================================
// TlsSetting struct
// ============================================================================
void wrap_tls_setting(py::module& m) {
    py::class_<SimpleCommKitTlsSetting>(m, "TlsSetting",
        "TLS/SSL configuration for TCP connections.")
        .def(py::init<>())
        .def_readwrite("crt_file", &SimpleCommKitTlsSetting::crt_file,
            "Certificate file path (client cert, optional)")
        .def_readwrite("key_file", &SimpleCommKitTlsSetting::key_file,
            "Private key file path (optional)")
        .def_readwrite("ca_file", &SimpleCommKitTlsSetting::ca_file,
            "CA certificate file path (for verifying peer)")
        .def_readwrite("ca_path", &SimpleCommKitTlsSetting::ca_path,
            "CA certificates directory path")
        .def_readwrite("verify_peer", &SimpleCommKitTlsSetting::verify_peer,
            "Whether to verify the peer's certificate")
        .def("__repr__", [](const SimpleCommKitTlsSetting& s) {
            return "<TlsSetting ca='" + s.ca_file + "' verify=" +
                   std::string(s.verify_peer ? "true" : "false") + ">";
        });
}

// ============================================================================
// TcpClient class
// ============================================================================
void wrap_tcp_client(py::module& m) {
    py::class_<SimpleCommKitTcpClient>(m, "TcpClient",
        "TCP client for connecting to a remote TCP server.")

        .def(py::init<>(), "Create a new TcpClient instance.")

        // --- Connection management ---
        .def("connect", &SimpleCommKitTcpClient::connect,
            py::arg("host"), py::arg("port"),
            "Connect to a TCP server at host:port. Returns True on success.")
        .def("disconnect", &SimpleCommKitTcpClient::disconnect,
            "Disconnect from the server.")
        .def("is_connected", &SimpleCommKitTcpClient::isConnected,
            "Check if currently connected to the server.")

        // --- Data transmission ---
        .def("send", [](SimpleCommKitTcpClient& self, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.send(vec);
            },
            py::arg("data"),
            "Send binary data. Data is bytes. Returns bytes sent or error code.")
        .def("send_text", [](SimpleCommKitTcpClient& self, const std::string& data) -> int {
                return self.send(data);
            },
            py::arg("data"),
            "Send text (string) data. Returns bytes sent or error code.")

        // --- Configuration ---
        .def("set_connect_timeout", &SimpleCommKitTcpClient::setConnectTimeout,
            py::arg("timeout_ms"),
            "Set the connection timeout in milliseconds.")
        .def("set_reconnect", &SimpleCommKitTcpClient::setReconnect,
            py::arg("setting"),
            "Configure automatic reconnection using a TcpReconnectSetting.")
        .def("disable_reconnect", &SimpleCommKitTcpClient::disableReconnect,
            "Disable automatic reconnection.")

        // --- TLS / SSL ---
        .def("enable_tls", [](SimpleCommKitTcpClient& self, const SimpleCommKitTlsSetting& setting) -> bool {
                return self.enableTls(setting);
            },
            py::arg("setting"),
            "Enable TLS with custom certificate settings. Call BEFORE connect().")
        .def("enable_tls_default", [](SimpleCommKitTcpClient& self) -> bool {
                return self.enableTls();
            },
            "Enable TLS with default platform certificates. Call BEFORE connect().")

        // --- Callbacks ---
        .def("set_callback_on_connected",
            [](SimpleCommKitTcpClient& self, py::function callback) {
                self.setCallback_OnConnected([callback]() {
                    py::gil_scoped_acquire gil;
                    callback();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when the connection is established.")
        .def("set_callback_on_disconnected",
            [](SimpleCommKitTcpClient& self, py::function callback) {
                self.setCallback_OnDisconnected([callback]() {
                    py::gil_scoped_acquire gil;
                    callback();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when the connection is lost.")
        .def("set_callback_on_message",
            [](SimpleCommKitTcpClient& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.setCallback_OnMessage(
                    [cb](const std::vector<uint8_t>& payload) {
                        py::gil_scoped_acquire gil;
                        (*cb)(py::bytes(reinterpret_cast<const char*>(payload.data()), payload.size()));
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when data is received. Callback receives bytes payload.")
        .def("set_callback_on_error",
            [](SimpleCommKitTcpClient& self, py::function callback) {
                self.setCallback_OnError([callback](ErrorCode error_code) {
                    py::gil_scoped_acquire gil;
                    callback(error_code);
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback for error notifications. Callback receives an error_code (int).");
}

// ============================================================================
// TcpServer class
// ============================================================================
void wrap_tcp_server(py::module& m) {
    py::class_<SimpleCommKitTcpServer>(m, "TcpServer",
        "TCP server that accepts client connections.")

        .def(py::init<>(), "Create a new TcpServer instance.")

        // --- Lifecycle ---
        .def("start", &SimpleCommKitTcpServer::start,
            py::arg("port"), py::arg("host") = "0.0.0.0",
            "Start listening on host:port. Returns True on success.")
        .def("stop", &SimpleCommKitTcpServer::stop,
            "Stop the server and disconnect all clients.")
        .def("is_running", &SimpleCommKitTcpServer::isRunning,
            "Check if the server is currently running.")

        // --- Data transmission ---
        .def("send_to", [](SimpleCommKitTcpServer& self, uint32_t client_id, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.sendTo(client_id, vec);
            },
            py::arg("client_id"), py::arg("data"),
            "Send binary data to a specific client by ID. Returns bytes sent or error code.")
        .def("send_to_text", [](SimpleCommKitTcpServer& self, uint32_t client_id, const std::string& data) -> int {
                return self.sendTo(client_id, data);
            },
            py::arg("client_id"), py::arg("data"),
            "Send text data to a specific client by ID. Returns bytes sent or error code.")
        .def("broadcast", [](SimpleCommKitTcpServer& self, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.broadcast(vec);
            },
            py::arg("data"),
            "Broadcast binary data to all connected clients. Returns bytes sent or error code.")
        .def("broadcast_text", [](SimpleCommKitTcpServer& self, const std::string& data) -> int {
                return self.broadcast(data);
            },
            py::arg("data"),
            "Broadcast text data to all connected clients.")

        // --- Server info ---
        .def("connection_num", &SimpleCommKitTcpServer::connectionNum,
            "Get the current number of connected clients.")
        .def("port", &SimpleCommKitTcpServer::port,
            "Get the port the server is listening on.")
        .def("host", &SimpleCommKitTcpServer::host,
            "Get the host the server is bound to.")

        // --- Configuration ---
        .def("set_thread_num", &SimpleCommKitTcpServer::setThreadNum,
            py::arg("num"),
            "Set the number of I/O threads.")
        .def("set_max_connection_num", &SimpleCommKitTcpServer::setMaxConnectionNum,
            py::arg("num"),
            "Set the maximum number of concurrent connections.")

        // --- TLS / SSL ---
        .def("enable_tls", [](SimpleCommKitTcpServer& self, const SimpleCommKitTlsSetting& setting) -> bool {
                return self.enableTls(setting);
            },
            py::arg("setting"),
            "Enable TLS with custom certificate settings. Call BEFORE start().")
        .def("enable_tls_default", [](SimpleCommKitTcpServer& self) -> bool {
                return self.enableTls();
            },
            "Enable TLS with default platform certificates. Call BEFORE start().")

        // --- Callbacks ---
        .def("set_callback_on_client_connected",
            [](SimpleCommKitTcpServer& self, py::function callback) {
                self.setCallback_OnClientConnected([callback](uint32_t client_id) {
                    py::gil_scoped_acquire gil;
                    callback(client_id);
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when a client connects. Callback receives client_id.")
        .def("set_callback_on_client_disconnected",
            [](SimpleCommKitTcpServer& self, py::function callback) {
                self.setCallback_OnClientDisconnected([callback](uint32_t client_id) {
                    py::gil_scoped_acquire gil;
                    callback(client_id);
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when a client disconnects. Callback receives client_id.")
        .def("set_callback_on_message",
            [](SimpleCommKitTcpServer& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.setCallback_OnMessage(
                    [cb](uint32_t client_id, const std::vector<uint8_t>& payload) {
                        py::gil_scoped_acquire gil;
                        (*cb)(client_id, py::bytes(reinterpret_cast<const char*>(payload.data()), payload.size()));
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when data is received from a client. Callback receives (client_id, bytes).")
        .def("set_callback_on_error",
            [](SimpleCommKitTcpServer& self, py::function callback) {
                self.setCallback_OnError([callback](ErrorCode error_code) {
                    py::gil_scoped_acquire gil;
                    callback(error_code);
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback for error notifications. Callback receives an error_code (int).");
}

// ============================================================================
// Module entry point
// ============================================================================
PYBIND11_MODULE(_SimpleCommKitPyTcp, m) {
    m.doc() = R"pbdoc(
        SimpleCommKitPyTcp - Python bindings for SimpleCommKit TCP

        Provides cross-platform TCP client and server APIs for Python.
    )pbdoc";

    // Utility
    m.def("get_error_description", &SimpleCommKitErrorMap::GetErrorDescription,
          py::arg("error_code"),
          "Get a human-readable description for an error code.");

    m.def("get_version", &SimpleCommKit::getSimpleCommKitVersion,
        "Get the SimpleCommKit library version string.");

    // Wrap types
    wrap_reconnect_setting(m);
    wrap_tls_setting(m);

    // Wrap classes
    wrap_tcp_client(m);
    wrap_tcp_server(m);
}
