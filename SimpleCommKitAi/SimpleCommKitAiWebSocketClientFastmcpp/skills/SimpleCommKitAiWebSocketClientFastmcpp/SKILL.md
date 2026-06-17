---
name: simple-comm-kit-ai-ws-client-fastmcpp
description: Use the SimpleCommKitAiWebSocketClientFastmcpp MCP server to connect, send, and receive messages over WebSocket via native C++. This skill provides guidance on the recommended flow (connect -> send -> get_messages -> disconnect) and handles TLS, reconnection, and ping configuration. Use when the user wants to interact with WebSocket servers or debug WebSocket connections through the native C++ server.
---

# SimpleCommKitAiWebSocketClientFastmcpp (C++)

SimpleCommKitAiWebSocketClientFastmcpp is a native C++ AI-friendly WebSocket client toolkit powered by the SimpleCommKitWebSocket C++ library and the fastmcpp MCP framework. This skill provides instructions for using the SimpleCommKitAiWebSocketClientFastmcpp MCP server to interact with WebSocket servers directly from the host machine.

## Quick Start Flow

Always follow this sequence for WebSocket interactions:

1. **Connect**: Call `ws_connect` with a WebSocket URL (`ws://` or `wss://`).
2. **Configure** (optional): Call `ws_set_reconnect`, `ws_set_connect_timeout`, or `ws_set_ping_interval` as needed.
3. **Send**: Call `ws_send` to transmit data (text or hex bytes).
4. **Receive**: Call `ws_get_messages` to retrieve buffered messages.
5. **Disconnect**: Call `ws_disconnect` when finished.

## Core Instructions

- **URL Format**: WebSocket URLs use `ws://` (plain) or `wss://` (TLS). Example: `ws://127.0.0.1:8080/ws`, `wss://example.com/ws`.
- **Data Handling**: Binary data is returned as `data_hex` (always reliable) and `data_utf8` (convenience field). If the data is not valid UTF-8, invalid bytes are skipped, so `data_utf8` may be incomplete or empty. Use `data_hex` for protocol analysis and `data_utf8` for human-readable strings.
- **Send Data**: Data can be sent as text (UTF-8) or hex-encoded bytes (e.g. `"00FF"` or `"01 02 AA"`). Spaces are automatically stripped.
- **Reconnection**: Configure auto-reconnect with `ws_set_reconnect` to automatically recover from connection drops.
- **Ping**: Use `ws_set_ping_interval` to keep idle connections alive with periodic ping frames.
- **TLS**: The `wss://` protocol automatically enables TLS. For custom certificates, the underlying library supports TLS configuration.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `ws_connect` | Open a WebSocket connection | `url` (required) |
| `ws_disconnect` | Close the current connection | None |
| `ws_status` | Check connection state | None |
| `ws_send` | Send data to the server | `data` (required), `is_hex` (default: false) |
| `ws_get_messages` | Retrieve buffered received messages | None |
| `ws_set_reconnect` | Configure auto-reconnect | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` |
| `ws_set_connect_timeout` | Set connection timeout | `timeout_ms` (required) |
| `ws_set_ping_interval` | Set ping interval (0=disable) | `interval_ms` (required) |

## Additional Resources

- For detailed tool documentation and transport modes, see [the reference guide](references/REFERENCE.md).
- For concrete usage examples, see [examples.md](references/examples.md).
- For troubleshooting common issues, see [troubleshooting.md](references/troubleshooting.md).
