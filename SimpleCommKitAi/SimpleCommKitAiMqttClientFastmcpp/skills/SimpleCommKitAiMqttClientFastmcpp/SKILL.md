---
name: simple-comm-kit-ai-mqtt-client-fastmcpp
description: Use the SimpleCommKitAiMqttClientFastmcpp MCP server to connect to MQTT brokers, publish, and subscribe to MQTT topics via native C++. This skill provides guidance on the recommended flow (connect -> subscribe -> publish -> get_messages) and handles MQTT-specific features like QoS, retain, will messages, and reconnection. Use when the user wants to interact with MQTT brokers or IoT devices through the native C++ server.
---

# SimpleCommKitAiMqttClientFastmcpp (C++)

SimpleCommKitAiMqttClientFastmcpp is a native C++ AI-friendly MQTT client toolkit powered by the SimpleCommKitMqttClient C++ library and the fastmcpp MCP framework. This skill provides instructions for using the SimpleCommKitAiMqttClientFastmcpp MCP server to connect to MQTT brokers, publish and subscribe to topics, and interact with IoT devices.

## Quick Start Flow

Always follow this sequence for MQTT interactions:

1. **Connect**: Call `mqtt_connect` with a broker host and optional port, SSL, and credentials.
2. **Verify**: Call `mqtt_status` to confirm the connection is established.
3. **Subscribe**: Call `mqtt_subscribe` on topics you want to receive messages from.
4. **Publish**: Call `mqtt_publish` to send messages to topics.
5. **Retrieve**: Call `mqtt_get_messages` to get buffered incoming messages.
6. **Disconnect**: Call `mqtt_disconnect` when done to clean up the connection.

## Core Instructions

- **Connection**: Always connect before subscribing or publishing. Use `mqtt_status` to verify the connection state.
- **QoS Levels**: MQTT supports QoS 0 (at most once), 1 (at least once), and 2 (exactly once). Default is QoS 0.
- **Retain Flag**: Set `retain=true` when publishing to make the broker store the last message for new subscribers.
- **Data Handling**: Binary data can be sent as hex strings (set `is_hex=true`). Text data is sent as plain strings. Incoming messages include both `data_hex` (always reliable) and `data_utf8` (convenience field).
- **Message Buffering**: Incoming messages from subscribed topics are automatically buffered. Use `mqtt_get_messages` to retrieve and clear the buffer. Messages can be filtered by topic.
- **Will Messages**: Use `mqtt_set_will` before connecting to set a Last Will and Testament message that the broker publishes if the client disconnects unexpectedly.
- **Reconnection**: Use `mqtt_set_reconnect` before connecting to enable automatic reconnection on connection loss. Supports fixed, linear, and exponential backoff policies.
- **SSL/TLS**: Set `use_ssl=true` in `mqtt_connect` for secure connections (default port 8883 for SSL).

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `mqtt_connect` | Connect to MQTT broker | `host`*, `port`, `client_id`, `use_ssl`, `username`, `password` |
| `mqtt_disconnect` | Disconnect from broker | None |
| `mqtt_status` | Get connection status | None |
| `mqtt_publish` | Publish a message | `topic`*, `data`*, `qos`, `retain`, `is_hex` |
| `mqtt_subscribe` | Subscribe to a topic | `topic`*, `qos` |
| `mqtt_unsubscribe` | Unsubscribe from a topic | `topic`* |
| `mqtt_get_messages` | Retrieve buffered messages | `topic` (optional) |
| `mqtt_set_reconnect` | Configure reconnection | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` |
| `mqtt_set_will` | Set will message | `topic`*, `data`*, `qos`, `retain`, `is_hex` |
| `mqtt_set_auth` | Set credentials | `username`*, `password` |

* = required parameter

## Additional Resources

- For detailed tool documentation and QoS notes, see [the reference guide](references/REFERENCE.md).
- For concrete usage examples, see [examples.md](references/examples.md).
- For troubleshooting common issues, see [troubleshooting.md](references/troubleshooting.md).
