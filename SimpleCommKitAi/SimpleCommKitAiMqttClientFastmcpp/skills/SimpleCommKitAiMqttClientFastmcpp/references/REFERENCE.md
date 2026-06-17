# Reference Guide

## MQTT QoS Levels

| QoS | Name | Description |
|-----|------|-------------|
| 0 | At most once | Fire and forget. No acknowledgment. Fastest. |
| 1 | At least once | Guaranteed delivery but may duplicate. |
| 2 | Exactly once | Guaranteed delivery with no duplicates. Slowest. |

QoS is negotiated: the subscription QoS caps the published message QoS.

## Data Format

### Publishing

Publish supports two data modes:

- **Plain text** (`is_hex=false`, default): The `data` string is sent as-is (UTF-8 encoded).
- **Hex binary** (`is_hex=true`): The `data` string is interpreted as a hex string (e.g. `"00FF"` or `"01 02 AA"`).

### Retrieving Messages

Incoming messages are returned with:

- `topic`: The MQTT topic the message was published to
- `data_hex`: Hex string of raw bytes (always available)
- `data_utf8`: Best-effort UTF-8 interpretation (may be truncated or empty for binary data)
- `data_length`: Number of bytes in the payload

## Reconnection Policy

The `delay_policy` parameter in `mqtt_set_reconnect` controls the backoff strategy:

| Policy | Value | Description |
|--------|-------|-------------|
| Fixed | 0 | Always waits `min_delay_ms` between retries |
| Linear | 1 | Delay increases linearly: min → max |
| Exponential | 2+ | Delay doubles each retry: min → min*2 → min*4 → ... → max |

Set `max_retry_cnt=0` for unlimited retries.

## Will Messages (Last Will and Testament)

A will message is published by the broker when the client disconnects unexpectedly. Set it before connecting:

```json
{
  "mqtt_set_will": {
    "topic": "client/status",
    "data": "offline",
    "qos": 1,
    "retain": true
  }
}
```

## Transport Modes

### stdio (default)

Used by MCP clients that spawn the server process. JSON-RPC messages are exchanged via stdin/stdout.

```json
{
  "mcpServers": {
    "simplecommkitaimqttclient-fastmcpp": {
      "command": "simplecommkitaimqttclient-fastmcpp"
    }
  }
}
```

### SSE (Server-Sent Events)

Recommended for remote or web-based access.

```json
{
  "mcpServers": {
    "simplecommkitaimqttclient-fastmcpp": {
      "url": "http://localhost:8004/sse"
    }
  }
}
```

### Streamable HTTP

Modern transport with streaming support.

```json
{
  "mcpServers": {
    "simplecommkitaimqttclient-fastmcpp": {
      "url": "http://localhost:8004/mcp"
    }
  }
}
```

## Topic Best Practices

- Use hierarchical topic names: `home/livingroom/temperature`
- Avoid leading `/` in topic names
- MQTT wildcards:
  - `+` matches a single level: `home/+/temperature`
  - `#` matches all remaining levels: `home/#`
- Topic names are case-sensitive
