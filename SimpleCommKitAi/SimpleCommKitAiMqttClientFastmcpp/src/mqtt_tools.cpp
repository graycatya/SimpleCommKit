#include "mqtt_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace SimpleCommKitAiMqttClientFastmcpp
{

// ===========================================================================
// Schema builders
// ===========================================================================

static fastmcpp::Json prop_string(const char* desc,
                                   const char* default_val = nullptr)
{
    fastmcpp::Json p;
    p["type"] = "string";
    p["description"] = desc;
    if (default_val)
        p["default"] = default_val;
    return p;
}

static fastmcpp::Json prop_integer(const char* desc, int default_val = 0)
{
    fastmcpp::Json p;
    p["type"] = "integer";
    p["description"] = desc;
    p["default"] = default_val;
    return p;
}

static fastmcpp::Json prop_boolean(const char* desc, bool default_val)
{
    fastmcpp::Json p;
    p["type"] = "boolean";
    p["description"] = desc;
    p["default"] = default_val;
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
// Helper: content array wrapper for structured JSON results
// ===========================================================================

static fastmcpp::Json make_content(const fastmcpp::Json& data)
{
    return fastmcpp::Json{
        {"content",
         fastmcpp::Json::array(
             {fastmcpp::Json{{"type", "text"}, {"text", data.dump(2)}}})}};
}

// ===========================================================================
// Register all 10 MQTT MCP tools
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. mqtt_connect
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_connect",
        make_object_schema(
            {{"host", prop_string("MQTT broker hostname or IP address")},
             {"port", prop_integer("MQTT broker port (default 1883)", 1883)},
             {"client_id",
              prop_string("Client identifier (auto-generated if empty)", "")},
             {"use_ssl",
              prop_boolean("Use SSL/TLS connection (default false)", false)},
             {"username",
              prop_string("Username for authentication (optional)", "")},
             {"password",
              prop_string("Password for authentication (optional)", "")}},
            {"host"}),
        [](const Json& input) -> Json
        {
            auto& st = MqttState::instance();
            auto& mqtt = st.ensure_mqtt();

            // Disconnect if already connected
            if (st.connected_)
            {
                try
                {
                    mqtt.disconnect();
                }
                catch (...)
                {
                }
            }

            std::string host = input.value("host", "");
            int port = input.value("port", 1883);
            std::string client_id = input.value("client_id", "");
            bool use_ssl = input.value("use_ssl", false);
            std::string username = input.value("username", "");
            std::string password = input.value("password", "");

            // Store connection info
            st.current_host_ = host;
            st.current_port_ = port;
            st.current_ssl_ = use_ssl;
            st.current_client_id_ = client_id;
            st.current_username_ = username;
            st.current_password_ = password;

            // Set client ID if provided
            if (!client_id.empty())
                mqtt.setClientId(client_id);

            // Set auth if provided
            if (!username.empty())
                mqtt.setAuth(username, password);

            // Connect (SSL or plain)
            bool ok = use_ssl ? mqtt.connectSsl(host, port)
                              : mqtt.connect(host, port);

            if (ok)
                st.connected_ = true;
            else
                st.connected_ = false;

            return Json{{"success", ok},
                        {"host", host},
                        {"port", port},
                        {"ssl", use_ssl},
                        {"message",
                         ok ? "Connected to " + host + ":" + std::to_string(port)
                            : "Failed to connect to " + host + ":" +
                                  std::to_string(port)}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Connect to an MQTT broker. Supports plain TCP and SSL/TLS. "
            "Optionally set client ID, username, and password for "
            "authentication."});

    // -----------------------------------------------------------------------
    // 2. mqtt_disconnect
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_disconnect",
        [](const Json&) -> Json
        {
            auto& st = MqttState::instance();
            auto& mqtt = st.ensure_mqtt();

            if (!st.connected_)
                return Json{{"message", "Not connected"}};

            mqtt.disconnect();
            st.connected_ = false;
            st.clear_message_buffers();

            return Json{{"message", "Disconnected from MQTT broker"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Disconnect from the MQTT broker and clear buffered messages."});

    // -----------------------------------------------------------------------
    // 3. mqtt_status
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_status",
        [](const Json&) -> Json
        {
            auto& st = MqttState::instance();
            auto& mqtt = st.ensure_mqtt();

            bool connected = mqtt.isConnected();

            Json result{
                {"connected", connected},
                {"host", st.current_host_},
                {"port", st.current_port_},
                {"ssl", st.current_ssl_},
            };

            if (!st.current_client_id_.empty())
                result["client_id"] = st.current_client_id_;
            if (!st.current_username_.empty())
                result["username"] = st.current_username_;

            return result;
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Get the current MQTT connection status and broker info."});

    // -----------------------------------------------------------------------
    // 4. mqtt_publish
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_publish",
        make_object_schema(
            {{"topic", prop_string("MQTT topic to publish to")},
             {"data", prop_string("Payload data (plain text or hex string)")},
             {"qos", prop_integer("Quality of Service level (0, 1, or 2)", 0)},
             {"retain", prop_boolean("Retain message on broker", false)},
             {"is_hex",
              prop_boolean(
                  "If true, data is interpreted as a hex string (e.g. '00FF')",
                  false)}},
            {"topic", "data"}),
        [](const Json& input) -> Json
        {
            auto& st = MqttState::instance();
            auto& mqtt = st.ensure_mqtt();

            if (!st.connected_)
                throw std::runtime_error(
                    "Not connected. Call mqtt_connect first.");

            std::string topic = input.value("topic", "");
            std::string data_str = input.value("data", "");
            int qos = input.value("qos", 0);
            bool retain = input.value("retain", false);
            bool is_hex = input.value("is_hex", false);

            int result = 0;

            if (is_hex)
            {
                auto data_bytes = hex_to_bytes(data_str);
                result = mqtt.publish(topic, data_bytes, qos, retain);
                return Json{{"success", result == 0},
                            {"topic", topic},
                            {"qos", qos},
                            {"retain", retain},
                            {"bytes_published", data_bytes.size()},
                            {"message",
                             result == 0
                                 ? "Published " +
                                       std::to_string(data_bytes.size()) +
                                       " bytes to " + topic
                                 : "Publish failed with code " +
                                       std::to_string(result)}};
            }
            else
            {
                result = mqtt.publish(topic, data_str, qos, retain);
                return Json{
                    {"success", result == 0},
                    {"topic", topic},
                    {"qos", qos},
                    {"retain", retain},
                    {"bytes_published", data_str.size()},
                    {"message",
                     result == 0
                         ? "Published " + std::to_string(data_str.size()) +
                               " bytes to " + topic
                         : "Publish failed with code " +
                               std::to_string(result)}};
            }
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Publish a message to an MQTT topic. Data can be plain text or "
            "hex-encoded binary (set is_hex=true for binary). Supports QoS 0/1/2 "
            "and retain flag."});

    // -----------------------------------------------------------------------
    // 5. mqtt_subscribe
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_subscribe",
        make_object_schema(
            {{"topic", prop_string("MQTT topic to subscribe to")},
             {"qos",
              prop_integer("Maximum QoS level for this subscription", 0)}},
            {"topic"}),
        [](const Json& input) -> Json
        {
            auto& st = MqttState::instance();
            auto& mqtt = st.ensure_mqtt();

            if (!st.connected_)
                throw std::runtime_error(
                    "Not connected. Call mqtt_connect first.");

            std::string topic = input.value("topic", "");
            int qos = input.value("qos", 0);

            int result = mqtt.subscribe(topic, qos);

            return Json{{"success", result == 0},
                        {"topic", topic},
                        {"qos", qos},
                        {"message",
                         result == 0
                             ? "Subscribed to " + topic
                             : "Subscribe failed with code " +
                                   std::to_string(result)}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Subscribe to an MQTT topic to receive messages. "
            "Incoming messages are buffered and can be retrieved with "
            "mqtt_get_messages."});

    // -----------------------------------------------------------------------
    // 6. mqtt_unsubscribe
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_unsubscribe",
        make_object_schema(
            {{"topic",
              prop_string("MQTT topic to unsubscribe from")}},
            {"topic"}),
        [](const Json& input) -> Json
        {
            auto& st = MqttState::instance();
            auto& mqtt = st.ensure_mqtt();

            if (!st.connected_)
                throw std::runtime_error(
                    "Not connected. Call mqtt_connect first.");

            std::string topic = input.value("topic", "");

            int result = mqtt.unsubscribe(topic);

            return Json{{"success", result == 0},
                        {"topic", topic},
                        {"message",
                         result == 0
                             ? "Unsubscribed from " + topic
                             : "Unsubscribe failed with code " +
                                   std::to_string(result)}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Unsubscribe from an MQTT topic."});

    // -----------------------------------------------------------------------
    // 7. mqtt_get_messages
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_get_messages",
        make_object_schema(
            {{"topic",
              prop_string("Topic to retrieve messages for (empty = all topics)",
                          "")}}),
        [](const Json& input) -> Json
        {
            auto& st = MqttState::instance();

            std::string topic = input.value("topic", "");

            auto entries = st.drain_messages(topic);

            Json arr = Json::array();
            for (const auto& e : entries)
            {
                arr.push_back(Json{{"topic", e.topic},
                                   {"data_hex", e.data_hex},
                                   {"data_utf8", e.data_utf8},
                                   {"data_length", e.data_length}});
            }

            std::string summary = "Retrieved " + std::to_string(entries.size()) + " message(s).";
            return Json{
                {"content",
                 Json::array({Json{
                     {"type", "text"},
                     {"text", summary + "\n\n" + arr.dump(2)}
                 }})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Retrieve and clear buffered MQTT messages. "
            "Messages are buffered automatically after subscribing to topics. "
            "Leave topic empty to retrieve messages from all subscribed topics."});

    // -----------------------------------------------------------------------
    // 8. mqtt_set_reconnect
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_set_reconnect",
        make_object_schema(
            {{"min_delay_ms",
              prop_integer("Minimum reconnect delay in milliseconds", 1000)},
             {"max_delay_ms",
              prop_integer("Maximum reconnect delay in milliseconds", 10000)},
             {"delay_policy",
              prop_integer(
                  "Delay policy: 0=fixed, 1=linear, 2+=exponential (default 2)",
                  2)},
             {"max_retry_cnt",
              prop_integer("Maximum retry count (0 = unlimited)", 0)}}),
        [](const Json& input) -> Json
        {
            auto& st = MqttState::instance();
            auto& mqtt = st.ensure_mqtt();

            SimpleCommKit::SimpleCommKitMqttReconnectSetting setting;
            setting.min_delay_ms =
                static_cast<uint32_t>(input.value("min_delay_ms", 1000));
            setting.max_delay_ms =
                static_cast<uint32_t>(input.value("max_delay_ms", 10000));
            setting.delay_policy =
                static_cast<uint32_t>(input.value("delay_policy", 2));
            setting.max_retry_cnt =
                static_cast<uint32_t>(input.value("max_retry_cnt", 0));

            mqtt.setReconnect(setting);

            return Json{{"message", "Reconnect settings updated"},
                        {"min_delay_ms", setting.min_delay_ms},
                        {"max_delay_ms", setting.max_delay_ms},
                        {"delay_policy", setting.delay_policy},
                        {"max_retry_cnt", setting.max_retry_cnt}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Configure automatic reconnection behavior. "
            "Call before mqtt_connect to enable reconnection on connection "
            "loss."});

    // -----------------------------------------------------------------------
    // 9. mqtt_set_will
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_set_will",
        make_object_schema(
            {{"topic",
              prop_string("Will message topic")},
             {"data",
              prop_string("Will message payload (plain text or hex string)")},
             {"qos", prop_integer("Will message QoS level", 0)},
             {"retain", prop_boolean("Retain will message on broker", false)},
             {"is_hex",
              prop_boolean(
                  "If true, data is interpreted as a hex string (e.g. '00FF')",
                  false)}},
            {"topic", "data"}),
        [](const Json& input) -> Json
        {
            auto& st = MqttState::instance();
            auto& mqtt = st.ensure_mqtt();

            std::string topic = input.value("topic", "");
            std::string data_str = input.value("data", "");
            int qos = input.value("qos", 0);
            bool retain = input.value("retain", false);
            bool is_hex = input.value("is_hex", false);

            SimpleCommKit::SimpleCommKitMqttWillMessage will;
            will.topic = topic;
            will.qos = qos;
            will.retain = retain;

            if (is_hex)
                will.payload = hex_to_bytes(data_str);
            else
                will.payload.assign(data_str.begin(), data_str.end());

            mqtt.setWill(will);

            return Json{{"message",
                         "Will message set for topic '" + topic + "'"},
                        {"topic", topic},
                        {"qos", qos},
                        {"retain", retain}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Set a Last Will and Testament (LWT) message. "
            "This message is published by the broker if the client disconnects "
            "unexpectedly. Call before mqtt_connect."});

    // -----------------------------------------------------------------------
    // 10. mqtt_set_auth
    // -----------------------------------------------------------------------
    app.tool(
        "mqtt_set_auth",
        make_object_schema(
            {{"username", prop_string("Username for MQTT authentication")},
             {"password",
              prop_string("Password for MQTT authentication", "")}},
            {"username"}),
        [](const Json& input) -> Json
        {
            auto& st = MqttState::instance();
            auto& mqtt = st.ensure_mqtt();

            std::string username = input.value("username", "");
            std::string password = input.value("password", "");

            mqtt.setAuth(username, password);

            st.current_username_ = username;
            st.current_password_ = password;

            return Json{{"message", "Authentication credentials set"},
                        {"username", username}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Set MQTT authentication credentials (username and password). "
            "Call before mqtt_connect."});
}

} // namespace SimpleCommKitAiMqttClientFastmcpp
