---
name: simple-comm-kit-ai-udp-server
description: Use the SimpleCommKitAiUdpServer MCP server to start a UDP server, receive datagrams, and send replies. Use when the user wants to act as a UDP server.
---

# SimpleCommKitAiUdpServer

An AI-friendly UDP server toolkit. Use this skill to listen for datagrams, reply to senders, and broadcast.

## Quick Start Flow

1. **Start**: Call `udp_server_start` with port to start listening.
2. **Monitor**: Use `udp_server_status` to check server state.
3. **Send**: Use `udp_server_send_to` for specific hosts or `udp_server_broadcast` for broadcast.
4. **Receive**: Use `udp_server_get_messages` to retrieve buffered datagrams (includes sender host/port for easy reply).
5. **Stop**: Call `udp_server_stop` when done.

## Core Instructions

- **Reply**: Received messages include `from_host` and `from_port` for easy reply.
- **No Connection**: UDP is connectionless — both sides must be bound to their ports.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `udp_server_start` | Start UDP server | `port`, `host` |
| `udp_server_stop` | Stop UDP server | None |
| `udp_server_status` | Get server status | None |
| `udp_server_send_to` | Send to remote address | `host`, `port`, `data`, `is_hex` |
| `udp_server_broadcast` | Broadcast datagram | `data`, `is_hex` |
| `udp_server_get_messages` | Get buffered messages | None |
