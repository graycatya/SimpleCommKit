---
name: simple-comm-kit-ai-tcp-client-fastmcpp
description: >
  Use the SimpleCommKitAiTcpClientFastmcpp MCP server to connect, send, and receive
  data with TCP servers via native C++. This skill provides guidance on the
  recommended flow (connect -> send -> get_messages -> disconnect) for TCP client
  communication.
---

# SimpleCommKitAiTcpClientFastmcpp

Use this MCP server to interact with TCP servers from an AI agent. The server
exposes 6 MCP tools for connect/disconnect, send, receive, and reconnect
configuration, all backed by native C++ for low latency.

## Quick Start

1. **Connect** to a TCP server: `tcp_connect(host="127.0.0.1", port=8080)`
2. **Send** data: `tcp_send(data="Hello, Server!", is_hex=false)`
3. **Retrieve** received messages: `tcp_get_messages()`
4. **Check** connection status: `tcp_status()`
5. **Disconnect** when done: `tcp_disconnect()`

## Core Instructions

### Connecting
- Always connect before sending or receiving data.
- The client supports automatic reconnection if configured via `tcp_set_reconnect`.
- If reconnecting to a different server, call `tcp_disconnect()` first or use
  `tcp_connect()` to automatically disconnect and reconnect.

### Sending Data
- **Text mode** (`is_hex=false`, default): Data is sent as UTF-8 text.
- **Hex mode** (`is_hex=true`): Data is treated as a hex string
  (e.g. `"00FF"` or `"01 02 AA"`). Whitespace is ignored.

### Receiving Data
- All received data is automatically buffered via the `OnMessage` callback.
- Call `tcp_get_messages()` to retrieve and clear the buffer.
- Each message includes `data_hex`, `data_utf8`, and `data_length`.

### Reconnection
- Configure automatic reconnection with `tcp_set_reconnect`.
- **delay_policy**: `0` = fixed interval, `1` = linear backoff, `2+` = exponential.
- **max_retry_cnt**: `0` = unlimited retries.

### Status
- Use `tcp_status()` to check if the client is currently connected without
  side effects.

## Available Tools

| Tool | Parameters | Description |
|------|-----------|-------------|
| `tcp_connect` | `host` (str), `port` (int, default 8080) | Connect to a TCP server |
| `tcp_disconnect` | (none) | Disconnect from the server |
| `tcp_status` | (none) | Check connection status |
| `tcp_send` | `data` (str), `is_hex` (bool, default false) | Send data to the server |
| `tcp_get_messages` | (none) | Retrieve and clear buffered messages |
| `tcp_set_reconnect` | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` | Configure auto-reconnect |

## References

- [Reference Guide](references/REFERENCE.md) — Detailed API documentation and transport modes
- [Examples](references/examples.md) — Common usage examples
- [Troubleshooting](references/troubleshooting.md) — Common issues and solutions
