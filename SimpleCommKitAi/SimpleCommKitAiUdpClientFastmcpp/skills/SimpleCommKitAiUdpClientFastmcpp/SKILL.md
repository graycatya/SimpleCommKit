---
name: simple-comm-kit-ai-udp-client-fastmcpp
description: >
  Use the SimpleCommKitAiUdpClientFastmcpp MCP server to open, send, and receive
  datagrams with UDP sockets via native C++. This skill provides guidance on the
  recommended flow (open -> send_to -> get_messages -> close) for UDP client
  communication.
---

# SimpleCommKitAiUdpClientFastmcpp

Use this MCP server to interact with UDP sockets from an AI agent. The server
exposes 5 MCP tools for open/close, send, and receive datagrams, all backed by
native C++ for low latency.

## Quick Start

1. **Open** a local UDP socket: `udp_open(local_port=0, local_host="0.0.0.0")`
2. **Send** a datagram: `udp_send_to(host="127.0.0.1", port=8002, data="Hello!", is_hex=false)`
3. **Retrieve** received datagrams: `udp_get_messages()`
4. **Check** socket status: `udp_status()`
5. **Close** when done: `udp_close()`

## Core Instructions

### Opening a Socket
- UDP is connectionless — `udp_open()` only binds a local socket for sending/receiving.
- Set `local_port=0` to let the OS assign an available port.
- Opening a new socket when one is already open will automatically close the old one.

### Sending Datagrams
- **Text mode** (`is_hex=false`, default): Data is sent as UTF-8 text.
- **Hex mode** (`is_hex=true`): Data is treated as a hex string
  (e.g. `"00FF"` or `"01 02 AA"`). Whitespace is ignored.
- Each `udp_send_to()` call requires a target `host` and `port` — there is no persistent "connection".

### Receiving Datagrams
- All received data is automatically buffered via the `OnMessage` callback.
- Call `udp_get_messages()` to retrieve and clear the buffer.
- Each message includes `data_hex`, `data_utf8`, and `data_length`.

### Status
- Use `udp_status()` to check if the local socket is currently open without side effects.

## Available Tools

| Tool | Parameters | Description |
|------|-----------|-------------|
| `udp_open` | `local_port` (int, default 0), `local_host` (str, default "0.0.0.0") | Open a local UDP socket |
| `udp_close` | (none) | Close the UDP socket |
| `udp_status` | (none) | Check if the socket is open |
| `udp_send_to` | `host` (str), `port` (int), `data` (str), `is_hex` (bool, default false) | Send a datagram to target |
| `udp_get_messages` | (none) | Retrieve and clear buffered datagrams |

## References

- [Reference Guide](references/REFERENCE.md) — Detailed API documentation and transport modes
- [Examples](references/examples.md) — Common usage examples
- [Troubleshooting](references/troubleshooting.md) — Common issues and solutions
