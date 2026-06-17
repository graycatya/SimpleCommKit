# Reference Guide

## Data Format

### Hex Strings

The `ws_send` tool accepts hex-encoded data when `is_hex=true`. Accepted formats:

- Continuous: `"00FFAB12"`
- Spaced: `"00 FF AB 12"`
- Mixed case: `"00ffAb12"`

### Message Format

Received messages are returned as:
- `data_hex`: Hex string of raw bytes (always available)
- `data_utf8`: Best-effort UTF-8 interpretation (may be truncated if invalid)
- `data_length`: Number of bytes

## URL Format

WebSocket URLs follow the standard format:

```
ws://host:port/path     # Plain WebSocket
wss://host:port/path    # Secure WebSocket (TLS)
```

Examples:
- `ws://127.0.0.1:8080/ws` — Local server on port 8080
- `wss://echo.websocket.org` — Secure echo server
- `ws://192.168.1.100:9000/socket` — Custom path

## Reconnect Policy

The `ws_set_reconnect` tool configures automatic reconnection:

| Policy Value | Behavior |
|-------------|----------|
| 0 | Fixed delay (`min_delay_ms`) |
| 1 | Linear backoff (increases by `min_delay_ms` each retry) |
| 2+ | Exponential backoff (doubles each retry, capped at `max_delay_ms`) |

Setting `max_retry_cnt=0` means unlimited retries.

## Transport Modes

### stdio (default)

Used by MCP clients that spawn the server process. JSON-RPC messages are exchanged via stdin/stdout.

```json
{
  "mcpServers": {
    "simplecommkitaiwebsocketclient-fastmcpp": {
      "command": "simplecommkitaiwebsocketclient-fastmcpp"
    }
  }
}
```

### SSE (Server-Sent Events)

Recommended for remote or web-based access.

```json
{
  "mcpServers": {
    "simplecommkitaiwebsocketclient-fastmcpp": {
      "url": "http://localhost:8010/sse"
    }
  }
}
```

### Streamable HTTP

Modern transport with streaming support.

```json
{
  "mcpServers": {
    "simplecommkitaiwebsocketclient-fastmcpp": {
      "url": "http://localhost:8010/mcp"
    }
  }
}
```

### HTTP

Simple HTTP transport (non-streaming).

```json
{
  "mcpServers": {
    "simplecommkitaiwebsocketclient-fastmcpp": {
      "url": "http://localhost:8010"
    }
  }
}
```
