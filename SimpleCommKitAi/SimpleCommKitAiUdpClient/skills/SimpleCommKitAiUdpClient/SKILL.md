---
name: simple-comm-kit-ai-udp-client
description: Use the SimpleCommKitAiUdpClient MCP server to send and receive UDP datagrams. Use when the user wants to act as a UDP client.
---

# SimpleCommKitAiUdpClient

An AI-friendly UDP client toolkit. Use this skill to send datagrams and receive responses.

## Quick Start Flow

1. **Open**: Call `udp_open` to create a local UDP socket.
2. **Send**: Use `udp_send_to` to send datagrams to a specific host:port.
3. **Receive**: Use `udp_get_messages` to retrieve buffered datagrams.
4. **Close**: Call `udp_close` when done.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `udp_open` | Open UDP socket | `local_port`, `local_host` |
| `udp_close` | Close UDP socket | None |
| `udp_status` | Check socket status | None |
| `udp_send_to` | Send datagram | `host`, `port`, `data`, `is_hex` |
| `udp_get_messages` | Get buffered messages | None |
