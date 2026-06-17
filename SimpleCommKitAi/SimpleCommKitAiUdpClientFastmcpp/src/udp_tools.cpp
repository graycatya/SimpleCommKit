#include "udp_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace SimpleCommKitAiUdpClientFastmcpp
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
// Register all 5 UDP client MCP tools on a FastMCP application instance.
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. udp_open
    // -----------------------------------------------------------------------
    app.tool(
        "udp_open",
        make_object_schema({
            {"local_port", prop_integer("Local port to bind (0 = OS assigns a port)", 0)},
            {"local_host", prop_string("Local host address to bind (default '0.0.0.0')", "0.0.0.0")},
        }),
        [](const Json& input) -> Json
        {
            auto& state = UdpClientState::instance();
            state.ensure_client();

            int local_port        = input.value("local_port", 0);
            std::string local_host = input.value("local_host", "0.0.0.0");

            auto* client = state.get_client();
            if (!client)
                throw std::runtime_error("Failed to create UDP client");

            if (client->isOpen())
            {
                // Already open — close first
                client->close();
                state.clear_messages();
            }

            if (!client->open(local_port, local_host))
                throw std::runtime_error("Failed to open UDP socket on " +
                                         local_host + ":" + std::to_string(local_port));

            return Json{{"message", "UDP socket opened on " + local_host + ":" + std::to_string(local_port)},
                        {"local_host", local_host},
                        {"local_port", local_port}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Open a local UDP socket for sending and receiving datagrams. "
            "If a socket is already open, it is closed first. "
            "Set local_port=0 to let the OS assign an available port."});

    // -----------------------------------------------------------------------
    // 2. udp_close
    // -----------------------------------------------------------------------
    app.tool(
        "udp_close",
        [](const Json&) -> Json
        {
            auto& state  = UdpClientState::instance();
            auto* client = state.get_client();

            if (!client || !client->isOpen())
                return Json{{"message", "UDP socket is not open"}};

            client->close();
            state.clear_messages();

            return Json{{"message", "UDP socket closed"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Close the UDP socket. Buffered messages are cleared on close."});

    // -----------------------------------------------------------------------
    // 3. udp_status
    // -----------------------------------------------------------------------
    app.tool(
        "udp_status",
        [](const Json&) -> Json
        {
            auto& state  = UdpClientState::instance();
            auto* client = state.get_client();

            bool is_open = client ? client->isOpen() : false;

            return Json{{"is_open", is_open}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Check whether the UDP socket is currently open."});

    // -----------------------------------------------------------------------
    // 4. udp_send_to
    // -----------------------------------------------------------------------
    app.tool(
        "udp_send_to",
        make_object_schema({
            {"host",   prop_string ("Target hostname or IP address (e.g. '127.0.0.1' or '192.168.1.100')")},
            {"port",   prop_integer("Target port number")},
            {"data",   prop_string ("Data to send. If is_hex=false, sent as UTF-8 text. If is_hex=true, treated as hex bytes.")},
            {"is_hex", prop_boolean("If true, data is interpreted as a hex string (e.g. '00FF')", false)},
        }, {"host", "port", "data"}),
        [](const Json& input) -> Json
        {
            auto& state  = UdpClientState::instance();
            auto* client = state.get_client();

            if (!client)
            {
                state.ensure_client();
                client = state.get_client();
            }

            if (!client || !client->isOpen())
                throw std::runtime_error("UDP socket is not open. Call udp_open first.");

            std::string host = input.value("host", "");
            int port         = input.value("port", 0);
            std::string data = input.value("data", "");
            bool is_hex      = input.value("is_hex", false);

            int bytes_sent = 0;
            if (is_hex)
            {
                auto bytes = hex_to_bytes(data);
                bytes_sent = client->sendTo(host, port, bytes);
            }
            else
            {
                bytes_sent = client->sendTo(host, port, data);
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
            "Send a UDP datagram to the specified host and port. "
            "Data can be text (UTF-8) or hex-encoded bytes (e.g. '48 65 6C 6C 6F' for 'Hello'). "
            "The UDP socket must be open (call udp_open first)."});

    // -----------------------------------------------------------------------
    // 5. udp_get_messages
    // -----------------------------------------------------------------------
    app.tool(
        "udp_get_messages",
        [](const Json&) -> Json
        {
            auto& state   = UdpClientState::instance();
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
            "Retrieve and clear all buffered datagrams received by the UDP socket. "
            "Messages are automatically buffered via the OnMessage callback. "
            "Returns data as both hex and UTF-8 strings."});
}

} // namespace SimpleCommKitAiUdpClientFastmcpp
