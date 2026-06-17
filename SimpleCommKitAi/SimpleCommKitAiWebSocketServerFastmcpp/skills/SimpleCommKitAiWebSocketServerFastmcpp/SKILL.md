---
name: simple-comm-kit-ai-ws-server-fastmcpp
description: Use the SimpleCommKitAiWebSocketServerFastmcpp MCP server to start, stop, and manage a WebSocket server via native C++. This skill provides guidance on the recommended flow (start -> status -> send/broadcast -> get_messages -> stop) and handles client tracking, broadcasting, and server configuration. Use when the user wants to run a WebSocket server or manage WebSocket connections through the native C++ server.
---

# SimpleCommKitAiWebSocketServerFastmcpp (C++)

SimpleCommKitAiWebSocketServerFastmcpp is a native C++ AI-friendly WebSocket server toolkit powered by the SimpleCommKitWebSocket C++ library and the fastmcpp MCP framework. This skill provides instructions for using the SimpleCommKitAiWebSocketServerFastmcpp MCP server to run and manage a WebSocket server directly from the host machine.

## Quick Start Flow

Always follow this sequence for WebSocket server management:

1. **Start**: Call `ws_server_start` with a port and optional host.
2. **Verify**: Call `ws_server_status` to confirm the server is running and check connected clients.
3. **Communicate**: Use `ws_server_send` to send to a specific client, or `ws_server_broadcast` to send to all clients.
4. **Receive**: Call `ws_server_get_messages` to retrieve buffered messages from clients.
5. **Stop**: Call `ws_server_stop` when the server is no longer needed.

## Core Instructions

- **Port Selection**: Choose a port that is not in use by another application. Typical WebSocket ports are 8080 or 9000+.
- **Host Binding**: Use `"0.0.0.0"` to accept connections from any network interface, or `"127.0.0.1"` for local-only access.
- **Client Tracking**: The server tracks connected clients by `client_id`. Use `ws_server_status` to see how many clients are connected.
- **Data Handling**: Received data is returned as `data_hex` (always reliable) and `data_utf8` (convenience field). If the data is not valid UTF-8, invalid bytes are skipped.
- **Broadcasting**: `ws_server_broadcast` sends data to all connected clients at once. The return value indicates total bytes sent.
- **Configuration**: Use `ws_server_set_max_connections` and `ws_server_set_thread_num` BEFORE starting the server.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `ws_server_start` | Start the WebSocket server | `port` (required), `host` (default: "0.0.0.0") |
| `ws_server_stop` | Stop the WebSocket server | None |
| `ws_server_status` | Check server state and client count | None |
| `ws_server_send` | Send data to a specific client | `client_id` (required), `data` (required), `is_hex` (default: false) |
| `ws_server_broadcast` | Broadcast data to all clients | `data` (required), `is_hex` (default: false) |
| `ws_server_get_messages` | Retrieve buffered received messages | None |
| `ws_server_set_max_connections` | Set max concurrent connections | `max_connections` (required) |
| `ws_server_set_thread_num` | Set worker thread count | `thread_num` (required) |

## Additional Resources

- For detailed tool documentation and transport modes, see [the reference guide](references/REFERENCE.md).
- For concrete usage examples, see [examples.md](references/examples.md).
- For troubleshooting common issues, see [troubleshooting.md](references/troubleshooting.md).
