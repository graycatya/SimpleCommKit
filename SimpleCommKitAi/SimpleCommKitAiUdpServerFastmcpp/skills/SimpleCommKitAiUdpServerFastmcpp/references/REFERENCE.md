# Reference Guide — SimpleCommKitAiUdpServerFastmcpp

## Architecture

```
MCP Client (AI Agent)
        │
stdio / sse / streamable-http / http
        │
 ┌──────┴──────┐
 │   main.cpp   │  ← Entry point, CLI parsing, server startup
 ├──────────────┤
 │udp_srv_tools │  ← register_tools(): 6 MCP tools
 ├──────────────┤
 │udp_srv_state │  ← UdpServerState singleton: server + message buffer
 ├──────────────┤
 │   fastmcpp   │  ← C++ MCP framework
 ├──────────────┤
 │SimpleCommKitUdp│ ← C++ UDP abstraction (libhv backend)
 └──────────────┘
```

## Transport Modes

The server supports four transport modes via the `--transport` flag:

| Mode | Flag | Default Port | Notes |
|------|------|-------------|-------|
| stdio | (default) | N/A | Best for local process-based MCP clients |
| SSE | `--transport sse` | 8102 | Endpoints: `/sse`, `/messages` |
| Streamable HTTP | `--transport streamable-http` | 8102 | Endpoint: `/mcp` |
| HTTP | `--transport http` | 8102 | Standard HTTP POST |

### MCP Client Configuration Examples

**stdio:**
```json
{
  "mcpServers": {
    "udp-server": {
      "command": "simplecommkitaiupdserver-fastmcpp"
    }
  }
}
```

**SSE:**
```json
{
  "mcpServers": {
    "udp-server": {
      "url": "http://127.0.0.1:8102/sse"
    }
  }
}
```

**Streamable HTTP:**
```json
{
  "mcpServers": {
    "udp-server": {
      "url": "http://127.0.0.1:8102/mcp"
    }
  }
}
```

## Tool Reference

### 1. `udp_server_start`

Start listening for UDP datagrams on the given host and port.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `port` | integer | Yes | — | UDP port to listen on |
| `host` | string | No | "0.0.0.0" | Host address to bind |

**Returns:** `{"message", "host", "port"}`
**Note:** If already running, the server is stopped and restarted.

### 2. `udp_server_stop`

Stop the UDP server. Buffered messages are cleared.

**Returns:** `{"message"}`

### 3. `udp_server_status`

Check current server state. Has no side effects.

**Returns:** `{"running": true/false, "host": "...", "port": N}`

### 4. `udp_server_send_to`

Send a UDP datagram to a specific client address.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `host` | string | Yes | — | Target hostname or IP address |
| `port` | integer | Yes | — | Target port number |
| `data` | string | Yes | — | Data to send |
| `is_hex` | boolean | No | false | Interpret data as hex bytes |

**Returns:** `{"bytes_sent": N, "host": "...", "port": N}`
**Note:** Server must be running. Use this to reply to a specific client whose address you got from `udp_server_get_messages`.

### 5. `udp_server_broadcast`

Broadcast a datagram to all clients on the local network.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `data` | string | Yes | — | Data to broadcast |
| `is_hex` | boolean | No | false | Interpret data as hex bytes |

**Returns:** `{"bytes_sent": N, "message": "..."}`
**Note:** Sends to 255.255.255.255. Server must be running.

### 6. `udp_server_get_messages`

Retrieve all buffered received datagrams and clear the buffer.

**Returns:** `{"messages": [...], "count": N}`

Each message in the array:
```json
{
  "from_host": "192.168.1.100",
  "from_port": 12345,
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

- **Windows**: Supported (WinSock2 via libhv)
- **macOS**: Supported (BSD sockets via libhv)
- **Linux**: Supported (POSIX sockets via libhv)
