---
name: simple-comm-kit-ai-websocket-client
description: Use the SimpleCommKitAiWebSocketClient MCP server to connect to ws:// or wss:// endpoints, send messages, and receive real-time data. Use when the user wants to act as a WebSocket client.
---

# SimpleCommKitAiWebSocketClient

An AI-friendly WebSocket client toolkit. Use this skill to connect to WebSocket servers.

## Quick Start Flow

1. **Connect**: Call `ws_connect` with a WebSocket URL (ws:// or wss://).
2. **Send**: Use `ws_send` to send text or binary data.
3. **Receive**: Use `ws_get_messages` to retrieve buffered messages.
4. **Disconnect**: Call `ws_disconnect` when done.

## Core Instructions

- **URL Format**: Use `ws://` for plain WebSocket, `wss://` for secure WebSocket (TLS).
- **Data Format**: Use `is_hex=True` for binary hex, or `is_hex=False` (default) for text.
- **Reconnection**: Configure `ws_set_reconnect` for automatic reconnection.
- **Keep-Alive**: Use `ws_set_ping_interval` for ping/pong heartbeats.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `ws_connect` | Connect to WebSocket URL | `url` |
| `ws_disconnect` | Disconnect | None |
| `ws_status` | Check connection status | None |
| `ws_send` | Send message | `data`, `is_hex` |
| `ws_get_messages` | Get buffered messages | None |
| `ws_set_reconnect` | Configure auto-reconnect | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` |
| `ws_set_ping_interval` | Set ping interval (ms) | `ms` |
