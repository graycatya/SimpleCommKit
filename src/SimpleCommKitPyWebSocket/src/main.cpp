#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <memory>

#include <SimpleCommKitWebSocket.h>
#include <SimpleCommKitErrorMap.hpp>
#include <SimpleCommKitVersion.h>

namespace py = pybind11;
using namespace SimpleCommKit;

// ============================================================================
// WsReconnectSetting struct
// ============================================================================
void wrap_ws_reconnect_setting(py::module& m) {
    py::class_<SimpleCommKitWebSocketReconnectSetting>(m, "WsReconnectSetting",
        "Reconnect configuration for WebSocket client.")
        .def(py::init<>())
        .def_readwrite("min_delay_ms", &SimpleCommKitWebSocketReconnectSetting::min_delay_ms,
            "Minimum reconnect delay in milliseconds (default: 1000)")
        .def_readwrite("max_delay_ms", &SimpleCommKitWebSocketReconnectSetting::max_delay_ms,
            "Maximum reconnect delay in milliseconds (default: 10000)")
        .def_readwrite("delay_policy", &SimpleCommKitWebSocketReconnectSetting::delay_policy,
            "Delay policy: 0=fixed, 1=linear, 2+=exponential (default: 2)")
        .def_readwrite("max_retry_cnt", &SimpleCommKitWebSocketReconnectSetting::max_retry_cnt,
            "Maximum retry count (0 = unlimited)")
        .def("__repr__", [](const SimpleCommKitWebSocketReconnectSetting& s) {
            return "<WsReconnectSetting min=" + std::to_string(s.min_delay_ms) +
                   " max=" + std::to_string(s.max_delay_ms) +
                   " policy=" + std::to_string(s.delay_policy) +
                   " retries=" + std::to_string(s.max_retry_cnt) + ">";
        });
}

// ============================================================================
// WsTlsSetting struct
// ============================================================================
void wrap_ws_tls_setting(py::module& m) {
    py::class_<SimpleCommKitWebSocketTlsSetting>(m, "WsTlsSetting",
        "TLS/SSL configuration for WebSocket connections (wss://).")
        .def(py::init<>())
        .def_readwrite("crt_file", &SimpleCommKitWebSocketTlsSetting::crt_file,
            "Certificate file path (optional)")
        .def_readwrite("key_file", &SimpleCommKitWebSocketTlsSetting::key_file,
            "Private key file path (optional)")
        .def_readwrite("ca_file", &SimpleCommKitWebSocketTlsSetting::ca_file,
            "CA certificate file path (for verifying peer)")
        .def_readwrite("ca_path", &SimpleCommKitWebSocketTlsSetting::ca_path,
            "CA certificates directory path")
        .def_readwrite("verify_peer", &SimpleCommKitWebSocketTlsSetting::verify_peer,
            "Whether to verify the peer's certificate")
        .def("__repr__", [](const SimpleCommKitWebSocketTlsSetting& s) {
            return "<WsTlsSetting ca='" + s.ca_file + "' verify=" +
                   std::string(s.verify_peer ? "true" : "false") + ">";
        });
}

// ============================================================================
// WebSocketClient class
// ============================================================================
void wrap_ws_client(py::module& m) {
    py::class_<SimpleCommKitWebSocketClient>(m, "WebSocketClient",
        "WebSocket client for connecting to ws:// or wss:// endpoints.")

        .def(py::init<>(), "Create a new WebSocketClient instance.")

        // --- Connection management ---
        .def("open", &SimpleCommKitWebSocketClient::open,
            py::arg("url"),
            "Connect to a WebSocket server URL (e.g., ws://host:port/path or wss://host:port/path).")
        .def("close", &SimpleCommKitWebSocketClient::close,
            "Close the WebSocket connection.")
        .def("is_connected", &SimpleCommKitWebSocketClient::isConnected,
            "Check if currently connected.")

        // --- Data transmission ---
        .def("send", [](SimpleCommKitWebSocketClient& self, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.send(vec);
            },
            py::arg("data"),
            "Send binary data. Returns bytes sent or error code.")
        .def("send_text", [](SimpleCommKitWebSocketClient& self, const std::string& data) -> int {
                return self.send(data);
            },
            py::arg("data"),
            "Send text (string) data. Returns bytes sent or error code.")

        // --- Configuration ---
        .def("set_connect_timeout", &SimpleCommKitWebSocketClient::setConnectTimeout,
            py::arg("timeout_ms"),
            "Set the connection timeout in milliseconds.")
        .def("set_reconnect", &SimpleCommKitWebSocketClient::setReconnect,
            py::arg("setting"),
            "Configure automatic reconnection using a WsReconnectSetting.")
        .def("disable_reconnect", &SimpleCommKitWebSocketClient::disableReconnect,
            "Disable automatic reconnection.")
        .def("set_ping_interval", &SimpleCommKitWebSocketClient::setPingInterval,
            py::arg("ms"),
            "Set the ping interval in milliseconds.")

        // --- TLS / SSL ---
        .def("enable_tls", [](SimpleCommKitWebSocketClient& self, const SimpleCommKitWebSocketTlsSetting& setting) -> bool {
                return self.enableTls(setting);
            },
            py::arg("setting"),
            "Enable TLS with custom certificate settings. Call BEFORE open().")
        .def("enable_tls_default", [](SimpleCommKitWebSocketClient& self) -> bool {
                return self.enableTls();
            },
            "Enable TLS with default platform certificates. Call BEFORE open().")

        // --- Callbacks ---
        .def("set_callback_on_open",
            [](SimpleCommKitWebSocketClient& self, py::function callback) {
                self.setCallback_OnOpen([callback]() {
                    py::gil_scoped_acquire gil;
                    callback();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when the WebSocket connection is opened.")
        .def("set_callback_on_close",
            [](SimpleCommKitWebSocketClient& self, py::function callback) {
                self.setCallback_OnClose([callback]() {
                    py::gil_scoped_acquire gil;
                    callback();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when the WebSocket connection is closed.")
        .def("set_callback_on_message",
            [](SimpleCommKitWebSocketClient& self, py::function callback) {
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
            "Set callback invoked when a WebSocket message is received. Callback receives bytes payload.")
        .def("set_callback_on_error",
            [](SimpleCommKitWebSocketClient& self, py::function callback) {
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
// WebSocketServer class
// ============================================================================
void wrap_ws_server(py::module& m) {
    py::class_<SimpleCommKitWebSocketServer>(m, "WebSocketServer",
        "WebSocket server that accepts client connections.")

        .def(py::init<>(), "Create a new WebSocketServer instance.")

        // --- Lifecycle ---
        .def("start", &SimpleCommKitWebSocketServer::start,
            py::arg("port"), py::arg("host") = "0.0.0.0",
            "Start listening on host:port. Returns True on success.")
        .def("stop", &SimpleCommKitWebSocketServer::stop,
            "Stop the server and disconnect all clients.")
        .def("is_running", &SimpleCommKitWebSocketServer::isRunning,
            "Check if the server is currently running.")

        // --- Data transmission ---
        .def("send_to", [](SimpleCommKitWebSocketServer& self, uint32_t client_id, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.sendTo(client_id, vec);
            },
            py::arg("client_id"), py::arg("data"),
            "Send binary data to a specific client by ID. Returns bytes sent or error code.")
        .def("send_to_text", [](SimpleCommKitWebSocketServer& self, uint32_t client_id, const std::string& data) -> int {
                return self.sendTo(client_id, data);
            },
            py::arg("client_id"), py::arg("data"),
            "Send text data to a specific client by ID. Returns bytes sent or error code.")
        .def("broadcast", [](SimpleCommKitWebSocketServer& self, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.broadcast(vec);
            },
            py::arg("data"),
            "Broadcast binary data to all connected clients. Returns bytes sent or error code.")
        .def("broadcast_text", [](SimpleCommKitWebSocketServer& self, const std::string& data) -> int {
                return self.broadcast(data);
            },
            py::arg("data"),
            "Broadcast text data to all connected clients.")

        // --- Server info ---
        .def("connection_num", &SimpleCommKitWebSocketServer::connectionNum,
            "Get the current number of connected clients.")
        .def("port", &SimpleCommKitWebSocketServer::port,
            "Get the port the server is listening on.")
        .def("host", &SimpleCommKitWebSocketServer::host,
            "Get the host the server is bound to.")

        // --- Configuration ---
        .def("set_thread_num", &SimpleCommKitWebSocketServer::setThreadNum,
            py::arg("num"),
            "Set the number of I/O threads.")
        .def("set_max_connection_num", &SimpleCommKitWebSocketServer::setMaxConnectionNum,
            py::arg("num"),
            "Set the maximum number of concurrent connections.")

        // --- TLS / SSL ---
        .def("enable_tls", [](SimpleCommKitWebSocketServer& self, const SimpleCommKitWebSocketTlsSetting& setting) -> bool {
                return self.enableTls(setting);
            },
            py::arg("setting"),
            "Enable TLS with custom certificate settings. Call BEFORE start().")
        .def("enable_tls_default", [](SimpleCommKitWebSocketServer& self) -> bool {
                return self.enableTls();
            },
            "Enable TLS with default platform certificates. Call BEFORE start().")

        // --- Callbacks ---
        .def("set_callback_on_client_connected",
            [](SimpleCommKitWebSocketServer& self, py::function callback) {
                self.setCallback_OnClientConnected([callback](uint32_t client_id) {
                    py::gil_scoped_acquire gil;
                    callback(client_id);
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when a client connects. Callback receives client_id.")
        .def("set_callback_on_client_disconnected",
            [](SimpleCommKitWebSocketServer& self, py::function callback) {
                self.setCallback_OnClientDisconnected([callback](uint32_t client_id) {
                    py::gil_scoped_acquire gil;
                    callback(client_id);
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when a client disconnects. Callback receives client_id.")
        .def("set_callback_on_message",
            [](SimpleCommKitWebSocketServer& self, py::function callback) {
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
            "Set callback invoked when a message is received from a client. Callback receives (client_id, bytes).")
        .def("set_callback_on_error",
            [](SimpleCommKitWebSocketServer& self, py::function callback) {
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
PYBIND11_MODULE(_SimpleCommKitPyWebSocket, m) {
    m.doc() = R"pbdoc(
        SimpleCommKitPyWebSocket - Python bindings for SimpleCommKit WebSocket

        Provides cross-platform WebSocket client and server APIs for Python.
    )pbdoc";

    // Utility
    m.def("get_error_description", &SimpleCommKitErrorMap::GetErrorDescription,
          py::arg("error_code"),
          "Get a human-readable description for an error code.");

    m.def("get_version", []() {
        return SIMPLECOMMKIT_VERSION;
    }, "Get the SimpleCommKit library version string.");

    // Wrap types
    wrap_ws_reconnect_setting(m);
    wrap_ws_tls_setting(m);

    // Wrap classes
    wrap_ws_client(m);
    wrap_ws_server(m);
}
