#include "tcp_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace SimpleCommKitAiTcpClientFastmcpp
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
// Register all 6 TCP client MCP tools on a FastMCP application instance.
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. tcp_connect
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_connect",
        make_object_schema({
            {"host", prop_string("Hostname or IP address of the TCP server (e.g. '127.0.0.1')")},
            {"port", prop_integer("Port number of the TCP server (default 8080)", 8080)},
        }, {"host"}),
        [](const Json& input) -> Json
        {
            auto& state = TcpClientState::instance();
            state.ensure_client();

            std::string host = input.value("host", "");
            int port         = input.value("port", 8080);

            auto* client = state.get_client();
            if (!client)
                throw std::runtime_error("Failed to create TCP client");

            if (client->isConnected())
            {
                // Already connected — disconnect first
                client->disconnect();
                state.clear_messages();
            }

            if (!client->connect(host, port))
                throw std::runtime_error("Failed to connect to " + host + ":" +
                                         std::to_string(port));

            return Json{{"message", "Connected to " + host + ":" + std::to_string(port)},
                        {"host", host},
                        {"port", port}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Connect the TCP client to a remote server at the given host and port. "
            "If already connected, the existing connection is closed first."});

    // -----------------------------------------------------------------------
    // 2. tcp_disconnect
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_disconnect",
        [](const Json&) -> Json
        {
            auto& state  = TcpClientState::instance();
            auto* client = state.get_client();

            if (!client || !client->isConnected())
                return Json{{"message", "TCP client is not connected"}};

            client->disconnect();
            state.clear_messages();

            return Json{{"message", "Disconnected from server"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Disconnect the TCP client from the server. "
            "Buffered messages are cleared on disconnect."});

    // -----------------------------------------------------------------------
    // 3. tcp_status
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_status",
        [](const Json&) -> Json
        {
            auto& state  = TcpClientState::instance();
            auto* client = state.get_client();

            bool connected = client ? client->isConnected() : false;

            return Json{{"connected", connected}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Check whether the TCP client is currently connected to a server."});

    // -----------------------------------------------------------------------
    // 4. tcp_send
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_send",
        make_object_schema({
            {"data",   prop_string ("Data to send. If is_hex=false, sent as UTF-8 text. If is_hex=true, treated as hex bytes.")},
            {"is_hex", prop_boolean("If true, data is interpreted as a hex string (e.g. '00FF')", false)},
        }, {"data"}),
        [](const Json& input) -> Json
        {
            auto& state  = TcpClientState::instance();
            auto* client = state.get_client();

            if (!client)
            {
                state.ensure_client();
                client = state.get_client();
            }

            if (!client || !client->isConnected())
                throw std::runtime_error("TCP client is not connected");

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
            "Send data from the TCP client to the connected server. "
            "Data can be text (UTF-8) or hex-encoded bytes (e.g. '48 65 6C 6C 6F' for 'Hello')."});

    // -----------------------------------------------------------------------
    // 5. tcp_get_messages
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_get_messages",
        [](const Json&) -> Json
        {
            auto& state   = TcpClientState::instance();
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
            "Retrieve and clear all buffered messages received by the TCP client. "
            "Messages are automatically buffered via the OnMessage callback. "
            "Returns data as both hex and UTF-8 strings."});

    // -----------------------------------------------------------------------
    // 6. tcp_set_reconnect
    // -----------------------------------------------------------------------
    app.tool(
        "tcp_set_reconnect",
        make_object_schema({
            {"min_delay_ms",  prop_integer("Minimum reconnect delay in milliseconds (default 1000)", 1000)},
            {"max_delay_ms",  prop_integer("Maximum reconnect delay in milliseconds (default 10000)", 10000)},
            {"delay_policy",  prop_integer("Reconnect delay policy: 0=fixed, 1=linear, 2+=exponential (default 2)", 2)},
            {"max_retry_cnt", prop_integer("Maximum retry count: 0=unlimited (default 0)")},
        }),
        [](const Json& input) -> Json
        {
            auto& state  = TcpClientState::instance();
            state.ensure_client();

            auto* client = state.get_client();
            if (!client)
                throw std::runtime_error("Failed to create TCP client");

            SimpleCommKit::SimpleCommKitTcpReconnectSetting setting;
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
            "Configure automatic reconnection for the TCP client. "
            "Set min/max delay, backoff policy (0=fixed, 1=linear, 2+=exponential), "
            "and max retry count (0=unlimited)."});
}

} // namespace SimpleCommKitAiTcpClientFastmcpp
