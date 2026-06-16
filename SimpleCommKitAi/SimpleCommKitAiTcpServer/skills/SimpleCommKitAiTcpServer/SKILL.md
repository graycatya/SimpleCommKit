---
name: simple-comm-kit-ai-tcp-server
description: Use the SimpleCommKitAiTcpServer MCP server to start a TCP server, manage client connections, and send/receive data. Use when the user wants to act as a TCP server accepting client connections.
---

# SimpleCommKitAiTcpServer

An AI-friendly TCP server toolkit. Use this skill to start a TCP server, accept clients, broadcast messages.

## Quick Start Flow

1. **Start**: Call `tcp_server_start` with port to start listening.
2. **Monitor**: Use `tcp_server_status` to check connected clients.
3. **Send**: Use `tcp_server_send` for individual clients or `tcp_server_broadcast` for all.
4. **Receive**: Use `tcp_server_get_messages` to retrieve buffered messages.
5. **Stop**: Call `tcp_server_stop` when done.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `tcp_server_start` | Start TCP server | `port`, `host` |
| `tcp_server_stop` | Stop TCP server | None |
| `tcp_server_status` | Get server status | None |
| `tcp_server_send` | Send to a client | `client_id`, `data`, `is_hex` |
| `tcp_server_broadcast` | Broadcast to all | `data`, `is_hex` |
| `tcp_server_get_messages` | Get buffered messages | None |
