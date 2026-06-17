# Reference Guide — SimpleCommKitAiTcpClientFastmcpp

## Architecture

```
MCP Client (AI Agent)
        │
stdio / sse / streamable-http / http
        │
 ┌──────┴──────┐
 │   main.cpp   │  ← Entry point, CLI parsing, server startup
 ├──────────────┤
 │  tcp_tools   │  ← register_tools(): 6 MCP tools
 ├──────────────┤
 │  tcp_state    │  ← TcpClientState singleton: client + message buffer
 ├──────────────┤
 │   fastmcpp   │  ← C++ MCP framework
 ├──────────────┤
 │SimpleCommKitTcp│ ← C++ TCP abstraction (libhv backend)
 └──────────────┘
```

## Transport Modes

The server supports four transport modes via the `--transport` flag:

| Mode | Flag | Default Port | Notes |
|------|------|-------------|-------|
| stdio | (default) | N/A | Best for local process-based MCP clients |
| SSE | `--transport sse` | 8006 | Endpoints: `/sse`, `/messages` |
| Streamable HTTP | `--transport streamable-http` | 8006 | Endpoint: `/mcp` |
| HTTP | `--transport http` | 8006 | Standard HTTP POST |

### MCP Client Configuration Examples

**stdio:**
```json
{
  "mcpServers": {
    "tcp-client": {
      "command": "simplecommkitaitcpclient-fastmcpp"
    }
  }
}
```

**SSE:**
```json
{
  "mcpServers": {
    "tcp-client": {
      "url": "http://127.0.0.1:8006/sse"
    }
  }
}
```

**Streamable HTTP:**
```json
{
  "mcpServers": {
    "tcp-client": {
      "url": "http://127.0.0.1:8006/mcp"
    }
  }
}
```

## Tool Reference

### 1. `tcp_connect`

Connect the TCP client to a remote server.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `host` | string | Yes | — | Hostname or IP address |
| `port` | integer | No | 8080 | TCP port number |

**Returns:** `{"message", "host", "port"}`
**Note:** If already connected, disconnects first.

### 2. `tcp_disconnect`

Disconnect from the server. Buffered messages are cleared.

**Returns:** `{"message"}`

### 3. `tcp_status`

Check current connection state. Has no side effects.

**Returns:** `{"connected": true/false}`

### 4. `tcp_send`

Send data to the connected server.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `data` | string | Yes | — | Data to send |
| `is_hex` | boolean | No | false | Interpret data as hex bytes |

**Returns:** `{"bytes_sent": N}`
**Note:** Must be connected. In hex mode, supports whitespace-separated hex.

### 5. `tcp_get_messages`

Retrieve all buffered received messages and clear the buffer.

**Returns:** `{"messages": [...], "count": N}`

Each message in the array:
```json
{
  "data_hex": "48656C6C6F",
  "data_utf8": "Hello",
  "data_length": 5
}
```

### 6. `tcp_set_reconnect`

Configure automatic reconnection behavior.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `min_delay_ms` | integer | No | 1000 | Minimum delay between retries (ms) |
| `max_delay_ms` | integer | No | 10000 | Maximum delay between retries (ms) |
| `delay_policy` | integer | No | 2 | 0=fixed, 1=linear, 2+=exponential |
| `max_retry_cnt` | integer | No | 0 | Max retries (0=unlimited) |

**Returns:** Current reconnection settings.

## Data Formats

### Hex Strings
- Lowercase preferred: `"deadbeef"`
- Uppercase accepted: `"DEADBEEF"`
- Whitespace ignored: `"de ad be ef"`
- Must have even number of hex characters

### UTF-8
- Valid 1-4 byte UTF-8 sequences are preserved
- Invalid bytes are silently skipped in `bytes_to_utf8_safe()`
- ASCII (7-bit clean) passthrough

## Platform Support

- **Windows**: Supported (WinSock2 via libhv)
- **macOS**: Supported (BSD sockets via libhv)
- **Linux**: Supported (POSIX sockets via libhv)
