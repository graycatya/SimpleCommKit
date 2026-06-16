---
name: simple-comm-kit-ai-tcp-client
description: Use the SimpleCommKitAiTcpClient MCP server to connect, send, and receive data over TCP. This skill provides guidance on the recommended flow (connect -> send -> get_messages -> disconnect). Use when the user wants to act as a TCP client.
---

# SimpleCommKitAiTcpClient

An AI-friendly TCP client toolkit. Use this skill to connect to TCP servers, send and receive data.

## Quick Start Flow

1. **Connect**: Call `tcp_connect` with host and port.
2. **Send**: Use `tcp_send` to send data (text or hex).
3. **Receive**: Use `tcp_get_messages` to retrieve buffered messages.
4. **Disconnect**: Call `tcp_disconnect` when done.

## Core Instructions

- **Data Format**: Use `is_hex=True` to send raw binary as hex strings, or `is_hex=False` (default) for text.
- **Reconnection**: Configure `tcp_set_reconnect` for automatic reconnection with retry.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `tcp_connect` | Connect to TCP server | `host`, `port` |
| `tcp_disconnect` | Disconnect from server | None |
| `tcp_status` | Check connection status | None |
| `tcp_send` | Send data | `data`, `is_hex` |
| `tcp_get_messages` | Get buffered messages | None |
| `tcp_set_reconnect` | Configure auto-reconnect | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` |
