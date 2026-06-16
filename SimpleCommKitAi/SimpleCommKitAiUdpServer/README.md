# SimpleCommKitAiUdpServer

An AI-friendly UDP server toolkit powered by **SimpleCommKitPyUdp**.
Start a UDP server, receive datagrams and reply to senders from AI agents and scripts.

## Key Features

- **MCP Server**: Expose UDP server operations as MCP tools
- **HTTP Server**: Control UDP server over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiUdpServer
```

## MCP Server

```json
{ "command": "simplecommkitaiupdserver-mcp" }
```

## HTTP Server

```bash
simplecommkitaiupdserver-http --host 127.0.0.1 --port 8102
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| POST | `/start` | Start UDP server |
| POST | `/stop` | Stop UDP server |
| GET | `/status` | Get server status |
| POST | `/send` | Send to a remote address |
| POST | `/broadcast` | Broadcast datagram |
| GET | `/messages` | Get buffered messages |
| GET | `/events/stream` | SSE event stream |
