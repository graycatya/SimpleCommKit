#include "tcp_server_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace SimpleCommKitAiTcpServerFastmcpp
{

// ===========================================================================
// Schema builders — use nlohmann::json operator[] instead of initializer lists
// or raw string literals, both of which MSVC (locale 936) struggles with.
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
// Register all 6 TCP server MCP tools on a FastMCP application instance.
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. tcp_server_start
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_server_start",
        make_object_schema({
            {"port", prop_integer("Port number to listen on")},
            {"host", prop_string ("Host address to bind to (default 0.0.0.0)", "0.0.0.0")},
        }, {"port"}),
        [](const Json& input) -> Json
        {
            auto& state = TcpServerState::instance();
            state.ensure_server();

            int         port = input.value("port", 0);
            std::string host = input.value("host", "0.0.0.0");

            auto* server = state.get_server();
            if (!server)
                throw std::runtime_error("Failed to create TCP server");

            if (server->isRunning())
                throw std::runtime_error("Server is already running on port " +
                                         std::to_string(server->port()));

            if (!server->start(port, host))
                throw std::runtime_error("Server start failed on " + host + ":" +
                                         std::to_string(port));

            return Json{{"message", "Server started on " + host + ":" + std::to_string(port)},
                        {"host", host},
                        {"port", port}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Start a TCP server listening on the given port and host. "
            "Creates the server instance on first call and installs callbacks "
            "for client connect/disconnect and message reception."});

    // -----------------------------------------------------------------------
    // 2. tcp_server_stop
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_server_stop",
        [](const Json&) -> Json
        {
            auto& state  = TcpServerState::instance();
            auto* server = state.get_server();

            if (!server)
                return Json{{"message", "Server not initialized"}};

            if (!server->isRunning())
                return Json{{"message", "Server is not running"}};

            server->stop();
            state.clear_messages();

            return Json{{"message", "Server stopped"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Stop the running TCP server and disconnect all clients. "
            "Buffered messages are cleared on stop."});

    // -----------------------------------------------------------------------
    // 3. tcp_server_status
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_server_status",
        [](const Json&) -> Json
        {
            auto& state  = TcpServerState::instance();
            auto* server = state.get_server();

            if (!server)
                return Json{{"running", false},
                            {"connections", 0}};

            bool   running     = server->isRunning();
            size_t connections = running ? server->connectionNum() : 0;

            Json result{
                {"running", running},
                {"connections", connections},
            };

            if (running)
            {
                result["host"] = server->host();
                result["port"] = server->port();
            }

            return result;
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Check server status: whether it's running, current connection count, "
            "and the bound host/port."});

    // -----------------------------------------------------------------------
    // 4. tcp_server_send
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_server_send",
        make_object_schema({
            {"client_id", prop_integer("ID of the target client")},
            {"data",      prop_string ("Data to send. Text by default, hex bytes when is_hex=true.")},
            {"is_hex",    prop_boolean("If true, data is interpreted as a hex string (default false)", false)},
        }, {"client_id", "data"}),
        [](const Json& input) -> Json
        {
            auto& state  = TcpServerState::instance();
            auto* server = state.get_server();

            if (!server)
                throw std::runtime_error("Server not initialized");
            if (!server->isRunning())
                throw std::runtime_error("Server not running");

            uint32_t    client_id = static_cast<uint32_t>(input.value("client_id", 0));
            std::string data      = input.value("data", "");
            bool        is_hex    = input.value("is_hex", false);

            int bytes_sent = 0;
            if (is_hex)
            {
                auto bytes = hex_to_bytes(data);
                bytes_sent = server->sendTo(client_id, bytes);
            }
            else
            {
                bytes_sent = server->sendTo(client_id, data);
            }

            if (bytes_sent <= 0)
                throw std::runtime_error("Send to client " + std::to_string(client_id) +
                                         " failed");

            return Json{{"client_id", client_id},
                        {"bytes_sent", bytes_sent}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Send data to a specific connected client by client ID. "
            "Data can be text (UTF-8) or hex-encoded bytes."});

    // -----------------------------------------------------------------------
    // 5. tcp_server_broadcast
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_server_broadcast",
        make_object_schema({
            {"data",   prop_string ("Data to broadcast. Text by default, hex bytes when is_hex=true.")},
            {"is_hex", prop_boolean("If true, data is interpreted as a hex string (default false)", false)},
        }, {"data"}),
        [](const Json& input) -> Json
        {
            auto& state  = TcpServerState::instance();
            auto* server = state.get_server();

            if (!server)
                throw std::runtime_error("Server not initialized");
            if (!server->isRunning())
                throw std::runtime_error("Server not running");

            std::string data   = input.value("data", "");
            bool        is_hex = input.value("is_hex", false);

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
                throw std::runtime_error("Broadcast failed");

            return Json{{"bytes_sent", bytes_sent}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Broadcast data to all connected TCP clients. "
            "Data can be text (UTF-8) or hex-encoded bytes."});

    // -----------------------------------------------------------------------
    // 6. tcp_server_get_messages
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_server_get_messages",
        [](const Json&) -> Json
        {
            auto& state   = TcpServerState::instance();
            auto messages = state.drain_messages();

            Json arr = Json::array();
            for (const auto& m : messages)
            {
                arr.push_back(Json{{"client_id", m.client_id},
                                   {"data_hex", m.data_hex},
                                   {"data_utf8", m.data_utf8},
                                   {"data_length", m.data_length}});
            }

            return Json{{"messages", arr},
                        {"count", messages.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Retrieve and clear all buffered messages received from connected clients. "
            "Messages are automatically buffered via the OnMessage callback. "
            "Returns client_id, data_hex, data_utf8, and data_length for each message."});
}

} // namespace SimpleCommKitAiTcpServerFastmcpp
