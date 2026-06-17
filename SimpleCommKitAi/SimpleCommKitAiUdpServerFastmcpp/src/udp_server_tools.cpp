#include "udp_server_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace SimpleCommKitAiUdpServerFastmcpp
{

// ===========================================================================
// Schema builders
// ===========================================================================

static fastmcpp::Json prop_string(const char* desc, const char* default_val = nullptr)
{
    fastmcpp::Json p;
    p["type"]        = "string";
    p["description"] = desc;
    if (default_val)
        p["default"]  = default_val;
    return p;
}

static fastmcpp::Json prop_integer(const char* desc, int default_val = 0)
{
    fastmcpp::Json p;
    p["type"]        = "integer";
    p["description"] = desc;
    p["default"]     = default_val;
    return p;
}

static fastmcpp::Json prop_boolean(const char* desc, bool default_val)
{
    fastmcpp::Json p;
    p["type"]        = "boolean";
    p["description"] = desc;
    p["default"]     = default_val;
    return p;
}

static fastmcpp::Json make_object_schema(
    std::initializer_list<std::pair<const char*, fastmcpp::Json>> props,
    std::initializer_list<const char*> required = {})
{
    fastmcpp::Json schema;
    schema["type"] = "object";
    for (auto& kv : props)
        schema["properties"][kv.first] = kv.second;
    if (required.size() > 0)
        schema["required"] = required;
    return schema;
}

// ===========================================================================
// Register all 6 UDP server MCP tools on a FastMCP application instance.
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. udp_server_start
    // -----------------------------------------------------------------------
    app.tool(
        "udp_server_start",
        make_object_schema({
            {"port", prop_integer("UDP port to listen on")},
            {"host", prop_string("Host address to bind (default '0.0.0.0' listens on all interfaces)", "0.0.0.0")},
        }, {"port"}),
        [](const Json& input) -> Json
        {
            auto& state = UdpServerState::instance();
            state.ensure_server();

            int port             = input.value("port", 0);
            std::string host     = input.value("host", "0.0.0.0");

            auto* server = state.get_server();
            if (!server)
                throw std::runtime_error("Failed to create UDP server");

            if (server->isRunning())
            {
                server->stop();
                state.clear_messages();
            }

            if (!server->start(port, host))
                throw std::runtime_error("Failed to start UDP server on " +
                                         host + ":" + std::to_string(port));

            return Json{{"message", "UDP server started on " + host + ":" + std::to_string(port)},
                        {"host", host},
                        {"port", port}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Start the UDP server listening on the given host and port. "
            "All received datagrams are automatically buffered. "
            "If the server is already running, it is restarted first."});

    // -----------------------------------------------------------------------
    // 2. udp_server_stop
    // -----------------------------------------------------------------------
    app.tool(
        "udp_server_stop",
        [](const Json&) -> Json
        {
            auto& state   = UdpServerState::instance();
            auto* server  = state.get_server();

            if (!server || !server->isRunning())
                return Json{{"message", "UDP server is not running"}};

            server->stop();
            state.clear_messages();

            return Json{{"message", "UDP server stopped"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Stop the UDP server. Buffered messages are cleared on stop."});

    // -----------------------------------------------------------------------
    // 3. udp_server_status
    // -----------------------------------------------------------------------
    app.tool(
        "udp_server_status",
        [](const Json&) -> Json
        {
            auto& state   = UdpServerState::instance();
            auto* server  = state.get_server();

            bool running = server ? server->isRunning() : false;
            int  port    = (server && running) ? static_cast<int>(server->port()) : 0;
            std::string host = (server && running) ? server->host() : "";

            return Json{{"running", running},
                        {"host", host},
                        {"port", port}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Check whether the UDP server is currently listening. "
            "Returns the bound host and port if running."});

    // -----------------------------------------------------------------------
    // 4. udp_server_send_to
    // -----------------------------------------------------------------------
    app.tool(
        "udp_server_send_to",
        make_object_schema({
            {"host",   prop_string ("Target hostname or IP address (e.g. '127.0.0.1')")},
            {"port",   prop_integer("Target port number")},
            {"data",   prop_string ("Data to send. If is_hex=false, sent as UTF-8 text. If is_hex=true, treated as hex bytes.")},
            {"is_hex", prop_boolean("If true, data is interpreted as a hex string (e.g. '00FF')", false)},
        }, {"host", "port", "data"}),
        [](const Json& input) -> Json
        {
            auto& state   = UdpServerState::instance();
            auto* server  = state.get_server();

            if (!server)
            {
                state.ensure_server();
                server = state.get_server();
            }

            if (!server || !server->isRunning())
                throw std::runtime_error("UDP server is not running. Call udp_server_start first.");

            std::string host = input.value("host", "");
            int port         = input.value("port", 0);
            std::string data = input.value("data", "");
            bool is_hex      = input.value("is_hex", false);

            int bytes_sent = 0;
            if (is_hex)
            {
                auto bytes = hex_to_bytes(data);
                bytes_sent = server->sendTo(host, port, bytes);
            }
            else
            {
                bytes_sent = server->sendTo(host, port, data);
            }

            if (bytes_sent <= 0)
                throw std::runtime_error("Failed to send UDP datagram to " +
                                         host + ":" + std::to_string(port));

            return Json{{"bytes_sent", bytes_sent},
                        {"host", host},
                        {"port", port}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Send a UDP datagram from the server to a specific host and port. "
            "Data can be text (UTF-8) or hex-encoded bytes. "
            "The server must be running (call udp_server_start first)."});

    // -----------------------------------------------------------------------
    // 5. udp_server_broadcast
    // -----------------------------------------------------------------------
    app.tool(
        "udp_server_broadcast",
        make_object_schema({
            {"data",   prop_string ("Data to broadcast. If is_hex=false, sent as UTF-8 text. If is_hex=true, treated as hex bytes.")},
            {"is_hex", prop_boolean("If true, data is interpreted as a hex string (e.g. '00FF')", false)},
        }, {"data"}),
        [](const Json& input) -> Json
        {
            auto& state   = UdpServerState::instance();
            auto* server  = state.get_server();

            if (!server)
            {
                state.ensure_server();
                server = state.get_server();
            }

            if (!server || !server->isRunning())
                throw std::runtime_error("UDP server is not running. Call udp_server_start first.");

            std::string data = input.value("data", "");
            bool is_hex      = input.value("is_hex", false);

            int bytes_sent = 0;
            if (is_hex)
            {
                auto bytes = hex_to_bytes(data);
                bytes_sent = server->broadcast(bytes);
            }
            else
            {
                bytes_sent = server->broadcast(data);
            }

            if (bytes_sent <= 0)
                throw std::runtime_error("Failed to broadcast UDP datagram (no clients reachable)");

            return Json{{"bytes_sent", bytes_sent},
                        {"message", "Broadcast sent to 255.255.255.255"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Broadcast a UDP datagram to all clients on the local network (255.255.255.255). "
            "Data can be text (UTF-8) or hex-encoded bytes. "
            "The server must be running (call udp_server_start first)."});

    // -----------------------------------------------------------------------
    // 6. udp_server_get_messages
    // -----------------------------------------------------------------------
    app.tool(
        "udp_server_get_messages",
        [](const Json&) -> Json
        {
            auto& state   = UdpServerState::instance();
            auto messages = state.drain_messages();

            Json arr = Json::array();
            for (const auto& m : messages)
            {
                arr.push_back(Json{{"from_host", m.from_host},
                                   {"from_port", m.from_port},
                                   {"data_hex", m.data_hex},
                                   {"data_utf8", m.data_utf8},
                                   {"data_length", m.data_length}});
            }

            return Json{{"messages", arr},
                        {"count", messages.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Retrieve and clear all buffered datagrams received by the UDP server. "
            "Messages are automatically buffered via the OnMessage callback. "
            "Each message includes the sender's host and port, plus data in hex and UTF-8 formats."});
}

} // namespace SimpleCommKitAiUdpServerFastmcpp
