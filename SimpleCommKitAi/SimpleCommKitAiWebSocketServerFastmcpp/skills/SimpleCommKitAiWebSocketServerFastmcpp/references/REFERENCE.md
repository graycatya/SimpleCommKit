# Reference Guide

## Architecture

```
WebSocket Client ──┬── WebSocket Client
                   │
                   ▼
    ┌──────────────────────────────────┐
    │  SimpleCommKitWebSocketServer    │
    │  (libhv hv::WebSocketServer)     │
    ├──────────────────────────────────┤
    │  ws_server_start(port, host)     │
    │  ws_server_stop()                │
    │  sendTo(client_id, data)         │
    │  broadcast(data)                 │
    │  connectionNum() / port() / host│
    └──────────────────────────────────┘
```

## Data Format

### Hex Strings

The `ws_server_send` and `ws_server_broadcast` tools accept hex-encoded data when `is_hex=true`. Accepted formats:

- Continuous: `"00FFAB12"`
- Spaced: `"00 FF AB 12"`
- Mixed case: `"00ffAb12"`

### Message Format

Received messages are returned per-client:
- `client_id`: The client that sent the message
- `data_hex`: Hex string of raw bytes (always available)
- `data_utf8`: Best-effort UTF-8 interpretation
- `data_length`: Number of bytes

## Client IDs

WebSocket client IDs are assigned by the underlying libhv library as `uint32_t` values. They increment with each new connection.

## Configuration Parameters

| Tool | Default | Description |
|------|---------|-------------|
| `ws_server_set_max_connections` | 100 | Maximum concurrent client connections |
| `ws_server_set_thread_num` | System default | Worker threads for connection handling |

**Important**: Call configuration tools BEFORE `ws_server_start` for them to take effect.

## Transport Modes

### stdio (default)

Used by MCP clients that spawn the server process.

```json
{
  "mcpServers": {
    "simplecommkitaiwebsocketserver-fastmcpp": {
      "command": "simplecommkitaiwebsocketserver-fastmcpp"
    }
  }
}
```

### SSE (Server-Sent Events)

Recommended for remote access.

```json
{
  "mcpServers": {
    "simplecommkitaiwebsocketserver-fastmcpp": {
      "url": "http://localhost:8011/sse"
    }
  }
}
```

### Streamable HTTP

```json
{
  "mcpServers": {
    "simplecommkitaiwebsocketserver-fastmcpp": {
      "url": "http://localhost:8011/mcp"
    }
  }
}
```

### HTTP

```json
{
  "mcpServers": {
    "simplecommkitaiwebsocketserver-fastmcpp": {
      "url": "http://localhost:8011"
    }
  }
}
```
