#include "ws_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace SimpleCommKitAiWebSocketClientFastmcpp
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
// Register all 8 WebSocket client MCP tools on a FastMCP application instance.
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. ws_connect
    // -----------------------------------------------------------------------
    app.tool(
        "ws_connect",
        make_object_schema({
            {"url", prop_string("WebSocket URL to connect to (e.g. 'ws://127.0.0.1:8080/ws' or 'wss://example.com/ws')")},
        }, {"url"}),
        [](const Json& input) -> Json
        {
            auto& state = WsClientState::instance();
            state.ensure_client();

            std::string url = input.value("url", "");

            auto* client = state.get_client();
            if (!client)
                throw std::runtime_error("Failed to create WebSocket client");

            if (client->isConnected())
            {
                // Already connected — close first
                client->close();
                state.clear_messages();
            }

            if (!client->open(url))
                throw std::runtime_error("Failed to open WebSocket connection to " + url);

            return Json{{"message", "WebSocket connection opened to " + url},
                        {"url", url}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Open a WebSocket connection to the given URL. "
            "Supports ws:// (plain) and wss:// (TLS) protocols. "
            "If already connected, the existing connection is closed first."});

    // -----------------------------------------------------------------------
    // 2. ws_disconnect
    // -----------------------------------------------------------------------
    app.tool(
        "ws_disconnect",
        [](const Json&) -> Json
        {
            auto& state  = WsClientState::instance();
            auto* client = state.get_client();

            if (!client || !client->isConnected())
                return Json{{"message", "WebSocket client is not connected"}};

            client->close();
            state.clear_messages();

            return Json{{"message", "WebSocket connection closed"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Close the current WebSocket connection. "
            "Buffered messages are cleared on disconnect."});

    // -----------------------------------------------------------------------
    // 3. ws_status
    // -----------------------------------------------------------------------
    app.tool(
        "ws_status",
        [](const Json&) -> Json
        {
            auto& state  = WsClientState::instance();
            auto* client = state.get_client();

            bool connected = client ? client->isConnected() : false;

            return Json{{"connected", connected}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Check whether the WebSocket client is currently connected to a server."});

    // -----------------------------------------------------------------------
    // 4. ws_send
    // -----------------------------------------------------------------------
    app.tool(
        "ws_send",
        make_object_schema({
            {"data",   prop_string("Data to send. If is_hex=false, sent as UTF-8 text. If is_hex=true, treated as hex bytes.")},
            {"is_hex", prop_boolean("If true, data is interpreted as a hex string (e.g. '00FF')", false)},
        }, {"data"}),
        [](const Json& input) -> Json
        {
            auto& state  = WsClientState::instance();
            auto* client = state.get_client();

            if (!client)
            {
                state.ensure_client();
                client = state.get_client();
            }

            if (!client || !client->isConnected())
                throw std::runtime_error("WebSocket client is not connected");

            std::string data   = input.value("data", "");
            bool is_hex        = input.value("is_hex", false);

            int bytes_sent = 0;
            if (is_hex)
            {
                auto bytes = hex_to_bytes(data);
                bytes_sent = client->send(bytes);
            }
            else
            {
                bytes_sent = client->send(data);
            }

            if (bytes_sent <= 0)
                throw std::runtime_error("Failed to send data");

            return Json{{"bytes_sent", bytes_sent}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Send data from the WebSocket client to the connected server. "
            "Data can be text (UTF-8) or hex-encoded bytes (e.g. '48 65 6C 6C 6F' for 'Hello')."});

    // -----------------------------------------------------------------------
    // 5. ws_get_messages
    // -----------------------------------------------------------------------
    app.tool(
        "ws_get_messages",
        [](const Json&) -> Json
        {
            auto& state   = WsClientState::instance();
            auto messages = state.drain_messages();

            Json arr = Json::array();
            for (const auto& m : messages)
            {
                arr.push_back(Json{{"data_hex", m.data_hex},
                                   {"data_utf8", m.data_utf8},
                                   {"data_length", m.data_length}});
            }

            return Json{{"messages", arr},
                        {"count", messages.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Retrieve and clear all buffered messages received by the WebSocket client. "
            "Messages are automatically buffered via the OnMessage callback. "
            "Returns data as both hex and UTF-8 strings."});

    // -----------------------------------------------------------------------
    // 6. ws_set_reconnect
    // -----------------------------------------------------------------------
    app.tool(
        "ws_set_reconnect",
        make_object_schema({
            {"min_delay_ms",  prop_integer("Minimum reconnect delay in milliseconds (default 1000)", 1000)},
            {"max_delay_ms",  prop_integer("Maximum reconnect delay in milliseconds (default 10000)", 10000)},
            {"delay_policy",  prop_integer("Reconnect delay policy: 0=fixed, 1=linear, 2+=exponential (default 2)", 2)},
            {"max_retry_cnt", prop_integer("Maximum retry count: 0=unlimited (default 0)")},
        }),
        [](const Json& input) -> Json
        {
            auto& state  = WsClientState::instance();
            state.ensure_client();

            auto* client = state.get_client();
            if (!client)
                throw std::runtime_error("Failed to create WebSocket client");

            SimpleCommKit::SimpleCommKitWebSocketReconnectSetting setting;
            setting.min_delay_ms  = static_cast<uint32_t>(input.value("min_delay_ms", 1000));
            setting.max_delay_ms  = static_cast<uint32_t>(input.value("max_delay_ms", 10000));
            setting.delay_policy  = static_cast<uint32_t>(input.value("delay_policy", 2));
            setting.max_retry_cnt = static_cast<uint32_t>(input.value("max_retry_cnt", 0));

            client->setReconnect(setting);

            return Json{{"message", "Reconnect settings applied"},
                        {"min_delay_ms", setting.min_delay_ms},
                        {"max_delay_ms", setting.max_delay_ms},
                        {"delay_policy", setting.delay_policy},
                        {"max_retry_cnt", setting.max_retry_cnt}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Configure automatic reconnection for the WebSocket client. "
            "Set min/max delay, backoff policy (0=fixed, 1=linear, 2+=exponential), "
            "and max retry count (0=unlimited)."});

    // -----------------------------------------------------------------------
    // 7. ws_set_connect_timeout
    // -----------------------------------------------------------------------
    app.tool(
        "ws_set_connect_timeout",
        make_object_schema({
            {"timeout_ms", prop_integer("Connection timeout in milliseconds (default 5000)", 5000)},
        }, {"timeout_ms"}),
        [](const Json& input) -> Json
        {
            auto& state  = WsClientState::instance();
            state.ensure_client();

            auto* client = state.get_client();
            if (!client)
                throw std::runtime_error("Failed to create WebSocket client");

            int timeout_ms = input.value("timeout_ms", 5000);
            client->setConnectTimeout(timeout_ms);

            return Json{{"message", "Connect timeout set"},
                        {"timeout_ms", timeout_ms}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Set the WebSocket connection timeout in milliseconds. "
            "This determines how long the client waits for a connection to be established. "
            "Default is 5000ms."});

    // -----------------------------------------------------------------------
    // 8. ws_set_ping_interval
    // -----------------------------------------------------------------------
    app.tool(
        "ws_set_ping_interval",
        make_object_schema({
            {"interval_ms", prop_integer("Ping interval in milliseconds. Set to 0 to disable pings (default 0).")},
        }, {"interval_ms"}),
        [](const Json& input) -> Json
        {
            auto& state  = WsClientState::instance();
            state.ensure_client();

            auto* client = state.get_client();
            if (!client)
                throw std::runtime_error("Failed to create WebSocket client");

            int interval_ms = input.value("interval_ms", 0);
            client->setPingInterval(interval_ms);

            if (interval_ms == 0)
                return Json{{"message", "Ping interval disabled"}};
            else
                return Json{{"message", "Ping interval set"},
                            {"interval_ms", interval_ms}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Set the WebSocket ping interval in milliseconds. "
            "When enabled, the client sends ping frames at the specified interval to "
            "keep the connection alive. Set to 0 to disable pings."});
}

} // namespace SimpleCommKitAiWebSocketClientFastmcpp
