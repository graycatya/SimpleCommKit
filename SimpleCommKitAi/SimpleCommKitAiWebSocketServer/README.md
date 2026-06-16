# SimpleCommKitAiWebSocketServer

An AI-friendly WebSocket server toolkit powered by **SimpleCommKitPyWebSocket**.
Start a WebSocket server, accept client connections, and send/receive messages from AI agents and scripts.

## Key Features

- **MCP Server**: Expose WebSocket server operations as MCP tools
- **HTTP Server**: Control WebSocket server over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux
- **TLS Support**: Built-in wss:// support

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiWebSocketServer
```

## MCP Server

```json
{ "command": "simplecommkitawebsocketserver-mcp" }
```

## HTTP Server

```bash
simplecommkitawebsocketserver-http --host 127.0.0.1 --port 8103
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| POST | `/start` | Start WebSocket server |
| POST | `/stop` | Stop WebSocket server |
| GET | `/status` | Get server status |
| GET | `/clients` | List connected clients |
| POST | `/send/{client_id}` | Send to a client |
| POST | `/broadcast` | Broadcast to all clients |
| GET | `/messages` | Get buffered messages |
| GET | `/events/stream` | SSE event stream |
