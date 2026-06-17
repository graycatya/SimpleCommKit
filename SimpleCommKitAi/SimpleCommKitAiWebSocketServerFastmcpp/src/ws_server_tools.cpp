#include "ws_server_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace SimpleCommKitAiWebSocketServerFastmcpp
{

// ===========================================================================
// Schema builders — use nlohmann::json operator[] for MSVC compatibility
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
// Register all 8 WebSocket server MCP tools on a FastMCP application instance.
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. ws_server_start
    // -----------------------------------------------------------------------
    app.tool(
        "ws_server_start",
        make_object_schema({
            {"port", prop_integer("Port number to listen on (default 8080)", 8080)},
            {"host", prop_string("Host address to bind to (default '0.0.0.0')", "0.0.0.0")},
        }, {"port"}),
        [](const Json& input) -> Json
        {
            auto& state = WsServerState::instance();
            state.ensure_server();

            int port         = input.value("port", 8080);
            std::string host = input.value("host", "0.0.0.0");

            auto* server = state.get_server();
            if (!server)
                throw std::runtime_error("Failed to create WebSocket server");

            if (server->isRunning())
            {
                // Already running — stop first
                server->stop();
                state.clear_messages();
            }

            if (!server->start(port, host))
                throw std::runtime_error("Failed to start WebSocket server on " +
                                         host + ":" + std::to_string(port));

            return Json{{"message", "WebSocket server started on " + host + ":" + std::to_string(port)},
                        {"host", host},
                        {"port", port}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Start the WebSocket server on the given host and port. "
            "If already running, the existing server is stopped first. "
            "Clients can connect via ws://host:port/path or wss:// if TLS is enabled."});

    // -----------------------------------------------------------------------
    // 2. ws_server_stop
    // -----------------------------------------------------------------------
    app.tool(
        "ws_server_stop",
        [](const Json&) -> Json
        {
            auto& state   = WsServerState::instance();
            auto* server  = state.get_server();

            if (!server || !server->isRunning())
                return Json{{"message", "WebSocket server is not running"}};

            server->stop();
            state.clear_messages();

            return Json{{"message", "WebSocket server stopped"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Stop the WebSocket server. All client connections will be closed "
            "and buffered messages cleared."});

    // -----------------------------------------------------------------------
    // 3. ws_server_status
    // -----------------------------------------------------------------------
    app.tool(
        "ws_server_status",
        [](const Json&) -> Json
        {
            auto& state   = WsServerState::instance();
            auto* server  = state.get_server();

            bool running = server ? server->isRunning() : false;
            size_t conns = running ? server->connectionNum() : 0;
            uint32_t p   = running ? server->port() : 0;
            std::string h = running ? server->host() : "";

            return Json{{"running", running},
                        {"connections", conns},
                        {"host", h},
                        {"port", p}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Check whether the WebSocket server is currently running, "
            "and get the number of connected clients, host, and port."});

    // -----------------------------------------------------------------------
    // 4. ws_server_send
    // -----------------------------------------------------------------------
    app.tool(
        "ws_server_send",
        make_object_schema({
            {"client_id", prop_integer("Client ID to send data to")},
            {"data",      prop_string("Data to send. If is_hex=false, sent as UTF-8 text. If is_hex=true, treated as hex bytes.")},
            {"is_hex",    prop_boolean("If true, data is interpreted as a hex string (e.g. '00FF')", false)},
        }, {"client_id", "data"}),
        [](const Json& input) -> Json
        {
            auto& state   = WsServerState::instance();
            auto* server  = state.get_server();

            if (!server)
            {
                state.ensure_server();
                server = state.get_server();
            }

            if (!server || !server->isRunning())
                throw std::runtime_error("WebSocket server is not running");

            uint32_t client_id = static_cast<uint32_t>(input.value("client_id", 0));
            std::string data   = input.value("data", "");
            bool is_hex        = input.value("is_hex", false);

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
                throw std::runtime_error("Failed to send data to client " +
                                         std::to_string(client_id));

            return Json{{"client_id", client_id},
                        {"bytes_sent", bytes_sent}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Send data to a specific WebSocket client identified by client_id. "
            "Data can be text (UTF-8) or hex-encoded bytes. "
            "Use ws_server_status to see connected client IDs."});

    // -----------------------------------------------------------------------
    // 5. ws_server_broadcast
    // -----------------------------------------------------------------------
    app.tool(
        "ws_server_broadcast",
        make_object_schema({
            {"data",   prop_string("Data to broadcast. If is_hex=false, sent as UTF-8 text. If is_hex=true, treated as hex bytes.")},
            {"is_hex", prop_boolean("If true, data is interpreted as a hex string (e.g. '00FF')", false)},
        }, {"data"}),
        [](const Json& input) -> Json
        {
            auto& state   = WsServerState::instance();
            auto* server  = state.get_server();

            if (!server)
            {
                state.ensure_server();
                server = state.get_server();
            }

            if (!server || !server->isRunning())
                throw std::runtime_error("WebSocket server is not running");

            std::string data   = input.value("data", "");
            bool is_hex        = input.value("is_hex", false);

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
                throw std::runtime_error("Failed to broadcast data");

            size_t conns = server->connectionNum();

            return Json{{"bytes_sent", bytes_sent},
                        {"clients", conns}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Broadcast data to all connected WebSocket clients. "
            "Data can be text (UTF-8) or hex-encoded bytes. "
            "Returns the total bytes sent and number of clients."});

    // -----------------------------------------------------------------------
    // 6. ws_server_get_messages
    // -----------------------------------------------------------------------
    app.tool(
        "ws_server_get_messages",
        [](const Json&) -> Json
        {
            auto& state   = WsServerState::instance();
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
            "Retrieve and clear all buffered messages received by the WebSocket server. "
            "Messages are automatically buffered via the OnMessage callback, "
            "tagged with the source client_id. Returns data as both hex and UTF-8 strings."});

    // -----------------------------------------------------------------------
    // 7. ws_server_set_max_connections
    // -----------------------------------------------------------------------
    app.tool(
        "ws_server_set_max_connections",
        make_object_schema({
            {"max_connections", prop_integer("Maximum number of concurrent client connections (default 100)", 100)},
        }, {"max_connections"}),
        [](const Json& input) -> Json
        {
            auto& state   = WsServerState::instance();
            state.ensure_server();

            auto* server  = state.get_server();
            if (!server)
                throw std::runtime_error("Failed to create WebSocket server");

            uint32_t max = static_cast<uint32_t>(input.value("max_connections", 100));
            server->setMaxConnectionNum(max);

            return Json{{"message", "Max connections set"},
                        {"max_connections", max}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Set the maximum number of concurrent WebSocket client connections. "
            "Call this before starting the server."});

    // -----------------------------------------------------------------------
    // 8. ws_server_set_thread_num
    // -----------------------------------------------------------------------
    app.tool(
        "ws_server_set_thread_num",
        make_object_schema({
            {"thread_num", prop_integer("Number of worker threads for the server (default depends on system)", 0)},
        }, {"thread_num"}),
        [](const Json& input) -> Json
        {
            auto& state   = WsServerState::instance();
            state.ensure_server();

            auto* server  = state.get_server();
            if (!server)
                throw std::runtime_error("Failed to create WebSocket server");

            int num = input.value("thread_num", 0);
            server->setThreadNum(num);

            return Json{{"message", "Thread number set"},
                        {"thread_num", num}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Set the number of worker threads for the WebSocket server. "
            "Call this before starting the server. Set to 0 for system default."});
}

} // namespace SimpleCommKitAiWebSocketServerFastmcpp
