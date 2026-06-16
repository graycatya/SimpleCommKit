#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <memory>

#include <SimpleCommKitUdp.h>
#include <SimpleCommKitErrorMap.hpp>
#include <SimpleCommKitVersion.h>

namespace py = pybind11;
using namespace SimpleCommKit;

// ============================================================================
// UdpClient class
// ============================================================================
void wrap_udp_client(py::module& m) {
    py::class_<SimpleCommKitUdpClient>(m, "UdpClient",
        "UDP client for sending and receiving datagrams.")

        .def(py::init<>(), "Create a new UdpClient instance.")

        // --- Lifecycle ---
        .def("open", &SimpleCommKitUdpClient::open,
            py::arg("local_port") = 0, py::arg("local_host") = "0.0.0.0",
            "Open the local UDP socket on local_port. Returns True on success.")
        .def("close", &SimpleCommKitUdpClient::close,
            "Close the UDP socket.")
        .def("is_open", &SimpleCommKitUdpClient::isOpen,
            "Check if the socket is currently open.")

        // --- Data transmission ---
        .def("send_to", [](SimpleCommKitUdpClient& self, const std::string& host, int port, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.sendTo(host, port, vec);
            },
            py::arg("host"), py::arg("port"), py::arg("data"),
            "Send binary data to a remote host:port. Returns bytes sent or error code.")
        .def("send_to_text", [](SimpleCommKitUdpClient& self, const std::string& host, int port, const std::string& data) -> int {
                return self.sendTo(host, port, data);
            },
            py::arg("host"), py::arg("port"), py::arg("data"),
            "Send text data to a remote host:port. Returns bytes sent or error code.")
        .def("set_remote_address", &SimpleCommKitUdpClient::setRemoteAddress,
            py::arg("host"), py::arg("port"),
            "Set a default remote address for subsequent send() calls.")
        .def("send", [](SimpleCommKitUdpClient& self, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.send(vec);
            },
            py::arg("data"),
            "Send binary data to the default remote address. Call set_remote_address() first.")
        .def("send_text", [](SimpleCommKitUdpClient& self, const std::string& data) -> int {
                return self.send(data);
            },
            py::arg("data"),
            "Send text data to the default remote address. Call set_remote_address() first.")

        // --- Configuration ---
        .def("set_read_timeout", &SimpleCommKitUdpClient::setReadTimeout,
            py::arg("timeout_ms"),
            "Set the read timeout in milliseconds.")

        // --- Callbacks ---
        .def("set_callback_on_message",
            [](SimpleCommKitUdpClient& self, py::function callback) {
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
            "Set callback invoked when a datagram is received. Callback receives bytes payload.")
        .def("set_callback_on_error",
            [](SimpleCommKitUdpClient& self, py::function callback) {
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
// UdpServer class
// ============================================================================
void wrap_udp_server(py::module& m) {
    py::class_<SimpleCommKitUdpServer>(m, "UdpServer",
        "UDP server that listens for incoming datagrams.")

        .def(py::init<>(), "Create a new UdpServer instance.")

        // --- Lifecycle ---
        .def("start", &SimpleCommKitUdpServer::start,
            py::arg("port"), py::arg("host") = "0.0.0.0",
            "Start listening on host:port. Returns True on success.")
        .def("stop", &SimpleCommKitUdpServer::stop,
            "Stop the server.")
        .def("is_running", &SimpleCommKitUdpServer::isRunning,
            "Check if the server is currently running.")

        // --- Data transmission ---
        .def("send_to", [](SimpleCommKitUdpServer& self, const std::string& host, int port, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.sendTo(host, port, vec);
            },
            py::arg("host"), py::arg("port"), py::arg("data"),
            "Send binary data to a specific remote host:port. Returns bytes sent or error code.")
        .def("send_to_text", [](SimpleCommKitUdpServer& self, const std::string& host, int port, const std::string& data) -> int {
                return self.sendTo(host, port, data);
            },
            py::arg("host"), py::arg("port"), py::arg("data"),
            "Send text data to a specific remote host:port. Returns bytes sent or error code.")
        .def("broadcast", [](SimpleCommKitUdpServer& self, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.broadcast(vec);
            },
            py::arg("data"),
            "Broadcast binary data to 255.255.255.255. Returns bytes sent or error code.")
        .def("broadcast_text", [](SimpleCommKitUdpServer& self, const std::string& data) -> int {
                return self.broadcast(data);
            },
            py::arg("data"),
            "Broadcast text data to 255.255.255.255.")

        // --- Server info ---
        .def("port", &SimpleCommKitUdpServer::port,
            "Get the port the server is listening on.")
        .def("host", &SimpleCommKitUdpServer::host,
            "Get the host the server is bound to.")

        // --- Callbacks ---
        .def("set_callback_on_message",
            [](SimpleCommKitUdpServer& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.setCallback_OnMessage(
                    [cb](const std::string& fromHost, int fromPort, const std::vector<uint8_t>& payload) {
                        py::gil_scoped_acquire gil;
                        (*cb)(fromHost, fromPort,
                              py::bytes(reinterpret_cast<const char*>(payload.data()), payload.size()));
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when a datagram is received. Callback receives (from_host, from_port, bytes).")
        .def("set_callback_on_error",
            [](SimpleCommKitUdpServer& self, py::function callback) {
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
PYBIND11_MODULE(_SimpleCommKitPyUdp, m) {
    m.doc() = R"pbdoc(
        SimpleCommKitPyUdp - Python bindings for SimpleCommKit UDP

        Provides cross-platform UDP client and server APIs for Python.
    )pbdoc";

    // Utility
    m.def("get_error_description", &SimpleCommKitErrorMap::GetErrorDescription,
          py::arg("error_code"),
          "Get a human-readable description for an error code.");

    m.def("get_version", []() {
        return SIMPLECOMMKIT_VERSION;
    }, "Get the SimpleCommKit library version string.");

    // Wrap classes
    wrap_udp_client(m);
    wrap_udp_server(m);
}
