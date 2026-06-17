# Reference Guide — SimpleCommKitAiUdpClientFastmcpp

## Architecture

```
MCP Client (AI Agent)
        │
stdio / sse / streamable-http / http
        │
 ┌──────┴──────┐
 │   main.cpp   │  ← Entry point, CLI parsing, server startup
 ├──────────────┤
 │  udp_tools   │  ← register_tools(): 5 MCP tools
 ├──────────────┤
 │  udp_state   │  ← UdpClientState singleton: client + message buffer
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
| SSE | `--transport sse` | 8008 | Endpoints: `/sse`, `/messages` |
| Streamable HTTP | `--transport streamable-http` | 8008 | Endpoint: `/mcp` |
| HTTP | `--transport http` | 8008 | Standard HTTP POST |

### MCP Client Configuration Examples

**stdio:**
```json
{
  "mcpServers": {
    "udp-client": {
      "command": "simplecommkitaiudpclient-fastmcpp"
    }
  }
}
```

**SSE:**
```json
{
  "mcpServers": {
    "udp-client": {
      "url": "http://127.0.0.1:8008/sse"
    }
  }
}
```

**Streamable HTTP:**
```json
{
  "mcpServers": {
    "udp-client": {
      "url": "http://127.0.0.1:8008/mcp"
    }
  }
}
```

## Tool Reference

### 1. `udp_open`

Open a local UDP socket for sending and receiving datagrams.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `local_port` | integer | No | 0 | Local port to bind (0 = OS assigns) |
| `local_host` | string | No | "0.0.0.0" | Local host address to bind |

**Returns:** `{"message", "local_host", "local_port"}`
**Note:** If a socket is already open, it is closed first.

### 2. `udp_close`

Close the UDP socket. Buffered messages are cleared.

**Returns:** `{"message"}`

### 3. `udp_status`

Check current socket state. Has no side effects.

**Returns:** `{"is_open": true/false}`

### 4. `udp_send_to`

Send a UDP datagram to the specified host and port.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `host` | string | Yes | — | Target hostname or IP address |
| `port` | integer | Yes | — | Target port number |
| `data` | string | Yes | — | Data to send |
| `is_hex` | boolean | No | false | Interpret data as hex bytes |

**Returns:** `{"bytes_sent": N, "host", "port"}`
**Note:** The socket must be open (call `udp_open` first). In hex mode, supports whitespace-separated hex.

### 5. `udp_get_messages`

Retrieve all buffered received datagrams and clear the buffer.

**Returns:** `{"messages": [...], "count": N}`

Each message in the array:
```json
{
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
