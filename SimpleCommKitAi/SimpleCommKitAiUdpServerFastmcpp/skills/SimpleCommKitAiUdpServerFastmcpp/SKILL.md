---
name: simple-comm-kit-ai-udp-server-fastmcpp
description: >
  Use the SimpleCommKitAiUdpServerFastmcpp MCP server to start, stop, send, broadcast,
  and receive datagrams via native C++. This skill provides guidance on the recommended
  flow (start -> send_to / broadcast -> get_messages -> stop) for UDP server management.
---

# SimpleCommKitAiUdpServerFastmcpp

Use this MCP server to manage a UDP server from an AI agent. The server
exposes 6 MCP tools for start/stop, send, broadcast, and receive datagrams,
all backed by native C++ for low latency.

## Quick Start

1. **Start** the UDP server: `udp_server_start(port=8002, host="0.0.0.0")`
2. **Check** status: `udp_server_status()`
3. **Send** to a specific client: `udp_server_send_to(host="127.0.0.1", port=8003, data="Hello!", is_hex=false)`
4. **Broadcast** to all clients: `udp_server_broadcast(data="Announcement")`
5. **Retrieve** received datagrams: `udp_server_get_messages()`
6. **Stop** when done: `udp_server_stop()`

## Core Instructions

### Starting the Server
- `udp_server_start(port, host)` binds the server to a local port and starts listening.
- Use `host="0.0.0.0"` (default) to listen on all network interfaces.
- If the server is already running, it is automatically restarted.

### Sending Datagrams
- **`udp_server_send_to(host, port, data)`** sends a datagram to a specific client address.
- **`udp_server_broadcast(data)`** sends to 255.255.255.255, reaching all devices on the local subnet.
- **Text mode** (`is_hex=false`, default): Data is sent as UTF-8 text.
- **Hex mode** (`is_hex=true`): Data is treated as a hex string (e.g. `"00FF"` or `"01 02 AA"`).

### Receiving Datagrams
- All received datagrams are automatically buffered via the `OnMessage` callback.
- Call `udp_server_get_messages()` to retrieve and clear the buffer.
- Each message includes the sender's `from_host`, `from_port`, plus `data_hex`, `data_utf8`, and `data_length`.

### Status
- Use `udp_server_status()` to check if the server is running, and get the bound host/port.

## Available Tools

| Tool | Parameters | Description |
|------|-----------|-------------|
| `udp_server_start` | `port` (int), `host` (str, default "0.0.0.0") | Start listening for UDP datagrams |
| `udp_server_stop` | (none) | Stop the UDP server |
| `udp_server_status` | (none) | Check running state and bound address |
| `udp_server_send_to` | `host` (str), `port` (int), `data` (str), `is_hex` (bool, default false) | Send datagram to a specific client |
| `udp_server_broadcast` | `data` (str), `is_hex` (bool, default false) | Broadcast to 255.255.255.255 |
| `udp_server_get_messages` | (none) | Retrieve and clear buffered datagrams |

## References

- [Reference Guide](references/REFERENCE.md) — Detailed API documentation and transport modes
- [Examples](references/examples.md) — Common usage examples
- [Troubleshooting](references/troubleshooting.md) — Common issues and solutions
