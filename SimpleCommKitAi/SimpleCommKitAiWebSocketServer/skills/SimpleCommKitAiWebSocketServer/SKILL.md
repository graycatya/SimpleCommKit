---
name: simple-comm-kit-ai-websocket-server
description: Use the SimpleCommKitAiWebSocketServer MCP server to start a WebSocket server, manage client connections, and broadcast messages. Use when the user wants to act as a WebSocket server.
---

# SimpleCommKitAiWebSocketServer

An AI-friendly WebSocket server toolkit. Use this skill to start a WebSocket server and manage real-time client connections.

## Quick Start Flow

1. **Start**: Call `ws_server_start` with port to start listening.
2. **Monitor**: Use `ws_server_status` to check connected clients.
3. **Send**: Use `ws_server_send` for individual clients or `ws_server_broadcast` for all.
4. **Receive**: Use `ws_server_get_messages` to retrieve buffered messages.
5. **Stop**: Call `ws_server_stop` when done.

## Core Instructions

- **Client IDs**: The server assigns numeric client IDs automatically on connection.
- **TLS**: Use the Python bindings directly for custom wss:// cert configuration.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `ws_server_start` | Start WebSocket server | `port`, `host` |
| `ws_server_stop` | Stop WebSocket server | None |
| `ws_server_status` | Get server status | None |
| `ws_server_send` | Send to a client | `client_id`, `data`, `is_hex` |
| `ws_server_broadcast` | Broadcast to all | `data`, `is_hex` |
| `ws_server_get_messages` | Get buffered messages | None |
