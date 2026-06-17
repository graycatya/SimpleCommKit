# Reference Guide — SimpleCommKitAiTcpServerFastmcpp

## Architecture

```
MCP Client (AI Agent)
        │
stdio / sse / streamable-http / http
        │
 ┌──────┴──────┐
 │   main.cpp   │  ← Entry point, CLI parsing, server startup
 ├──────────────┤
 │tcp_server_to│  ← register_tools(): 6 MCP tools
 │    ols       │
 ├──────────────┤
 │tcp_server_st│  ← TcpServerState singleton: server + message buffer
 │     ate      │
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
| SSE | `--transport sse` | 8007 | Endpoints: `/sse`, `/messages` |
| Streamable HTTP | `--transport streamable-http` | 8007 | Endpoint: `/mcp` |
| HTTP | `--transport http` | 8007 | Standard HTTP POST |

### MCP Client Configuration Examples

**stdio:**
```json
{
  "mcpServers": {
    "tcp-server": {
      "command": "simplecommkitaitcpserver-fastmcpp"
    }
  }
}
```

**SSE:**
```json
{
  "mcpServers": {
    "tcp-server": {
      "url": "http://127.0.0.1:8007/sse"
    }
  }
}
```

**Streamable HTTP:**
```json
{
  "mcpServers": {
    "tcp-server": {
      "url": "http://127.0.0.1:8007/mcp"
    }
  }
}
```

## Tool Reference

### 1. `tcp_server_start`

Start the TCP server listening on the given port and host.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `port` | integer | Yes | — | TCP port number to listen on |
| `host` | string | No | `"0.0.0.0"` | Host address to bind to |

**Returns:** `{"message", "host", "port"}`
**Note:** Creates the server instance lazily on first call. Cannot start if already running.

### 2. `tcp_server_stop`

Stop the running server and disconnect all clients. Buffered messages are cleared.

**Returns:** `{"message"}`

### 3. `tcp_server_status`

Check server state without side effects.

**Returns when not initialized:** `{"running": false, "connections": 0}`
**Returns when running:** `{"running": true, "connections": N, "host": "...", "port": N}`
**Returns when stopped:** `{"running": false, "connections": 0}`

### 4. `tcp_server_send`

Send data to a specific connected client by client ID.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `client_id` | integer | Yes | — | Target client ID |
| `data` | string | Yes | — | Data to send |
| `is_hex` | boolean | No | `false` | Interpret data as hex bytes |

**Returns:** `{"client_id": N, "bytes_sent": N}`
**Note:** Client IDs are obtained from `tcp_server_get_messages()` results or connection callbacks.

### 5. `tcp_server_broadcast`

Broadcast data to all connected clients.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `data` | string | Yes | — | Data to broadcast |
| `is_hex` | boolean | No | `false` | Interpret data as hex bytes |

**Returns:** `{"bytes_sent": N}`

### 6. `tcp_server_get_messages`

Retrieve all buffered messages received from clients and clear the buffer.

**Returns:** `{"messages": [...], "count": N}`

Each message in the array:
```json
{
  "client_id": 1,
  "data_hex": "48656C6C6F",
  "data_utf8": "Hello",
  "data_length": 5
}
```

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

- **Windows** ✓ (WinSock2 via libhv)
- **macOS** ✓ (BSD sockets via libhv)
- **Linux** ✓ (POSIX sockets via libhv)
