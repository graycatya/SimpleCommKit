---
name: simple-comm-kit-ai-tcp-server-fastmcpp
description: >
  Use the SimpleCommKitAiTcpServerFastmcpp MCP server to start, stop, and manage
  a TCP server via native C++. This skill provides guidance on the recommended
  flow (start -> send/broadcast -> get_messages -> stop) for TCP server management.
---

# SimpleCommKitAiTcpServerFastmcpp

Use this MCP server to manage a TCP server from an AI agent. The server
exposes 6 MCP tools for start/stop, send/broadcast, status, and message
retrieval, all backed by native C++ for low latency.

## Quick Start

1. **Start** the TCP server: `tcp_server_start(port=8080)`
2. **Check** status: `tcp_server_status()`
3. **Send** to a specific client: `tcp_server_send(client_id=1, data="Hello!")`
4. **Broadcast** to all clients: `tcp_server_broadcast(data="All clients, update!")`
5. **Retrieve** client messages: `tcp_server_get_messages()`
6. **Stop** when done: `tcp_server_stop()`

## Core Instructions

### Starting the Server
- Call `tcp_server_start(port, host)` to begin listening.
- The server is created lazily on first call — no need to initialize separately.
- Default host is `0.0.0.0` (all interfaces).
- Cannot start twice; check status first or stop before restarting.

### Sending Data
- **To a specific client**: Use `tcp_server_send(client_id, data, is_hex)`.
  Client IDs are obtained from message entries or the connected-clients callback.
- **To all clients**: Use `tcp_server_broadcast(data, is_hex)`.
- **Text mode** (`is_hex=false`, default): Data is sent as UTF-8 text.
- **Hex mode** (`is_hex=true`): Data is treated as a hex string
  (e.g. `"00FF"` or `"01 02 AA"`). Whitespace is ignored.

### Receiving Data
- All received data from clients is automatically buffered via the `OnMessage` callback.
- Each message includes `client_id`, `data_hex`, `data_utf8`, and `data_length`.
- Call `tcp_server_get_messages()` to retrieve and clear the buffer.

### Status
- Use `tcp_server_status()` to check if running, connection count, and bind address.

### Stopping
- `tcp_server_stop()` disconnects all clients, stops listening, and clears the message buffer.

## Available Tools

| Tool | Parameters | Description |
|------|-----------|-------------|
| `tcp_server_start` | `port` (int), `host` (str, default "0.0.0.0") | Start listening |
| `tcp_server_stop` | (none) | Stop server and disconnect clients |
| `tcp_server_status` | (none) | Check running state and connections |
| `tcp_server_send` | `client_id` (int), `data` (str), `is_hex` (bool) | Send to a specific client |
| `tcp_server_broadcast` | `data` (str), `is_hex` (bool) | Broadcast to all clients |
| `tcp_server_get_messages` | (none) | Retrieve buffered client messages |

## References

- [Reference Guide](references/REFERENCE.md) — Detailed API documentation and transport modes
- [Examples](references/examples.md) — Common usage examples
- [Troubleshooting](references/troubleshooting.md) — Common issues and solutions
