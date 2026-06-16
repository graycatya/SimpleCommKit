---
name: simple-comm-kit-ai-mqtt-client
description: Use the SimpleCommKitAiMqttClient MCP server to connect to MQTT brokers, publish messages, and subscribe to topics. Use when the user wants to interact with MQTT or IoT services.
---

# SimpleCommKitAiMqttClient

An AI-friendly MQTT client toolkit. Use this skill to connect to MQTT brokers, publish and subscribe to topics.

## Quick Start Flow

1. **Connect**: Call `mqtt_connect` with host, port, and optional client_id.
2. **Subscribe**: Use `mqtt_subscribe` to listen for messages on a topic.
3. **Publish**: Use `mqtt_publish` to send messages to a topic.
4. **Receive**: Use `mqtt_get_messages` to retrieve buffered messages.
5. **Disconnect**: Call `mqtt_disconnect` when done.

## Core Instructions

- **QoS Levels**: 0 (at most once), 1 (at least once), 2 (exactly once). Default is 0.
- **Retain**: Set `retain=true` for the broker to keep the last message on a topic.
- **Data Format**: Use `is_hex=True` for binary hex payloads, or `is_hex=False` (default) for text.
- **Client ID**: Set a unique `client_id` to maintain session state across reconnections.
- **Reconnection**: Use `mqtt_set_reconnect` for automatic reconnection.
- **TLS**: Pass `use_ssl=True` in `mqtt_connect` for MQTTS (port 8883 typically).

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `mqtt_connect` | Connect to MQTT broker | `host`, `port`, `client_id`, `use_ssl` |
| `mqtt_disconnect` | Disconnect from broker | None |
| `mqtt_status` | Check connection status | None |
| `mqtt_publish` | Publish to a topic | `topic`, `data`, `qos`, `retain`, `is_hex` |
| `mqtt_subscribe` | Subscribe to a topic | `topic`, `qos` |
| `mqtt_unsubscribe` | Unsubscribe from a topic | `topic` |
| `mqtt_get_messages` | Get buffered messages | None |
| `mqtt_set_reconnect` | Configure auto-reconnect | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` |
