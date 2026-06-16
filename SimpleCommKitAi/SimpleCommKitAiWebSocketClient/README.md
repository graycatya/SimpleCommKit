# SimpleCommKitAiWebSocketClient

An AI-friendly WebSocket client toolkit powered by **SimpleCommKitPyWebSocket**.
Connect to ws:// or wss:// servers, send and receive messages from AI agents and scripts.

## Key Features

- **MCP Server**: Expose WebSocket client operations as MCP tools
- **HTTP Server**: Control WebSocket client over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux
- **TLS Support**: Built-in wss:// support

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiWebSocketClient
```

## MCP Server

```json
{ "command": "simplecommkitawebsocketclient-mcp" }
```

## HTTP Server

```bash
simplecommkitawebsocketclient-http --host 127.0.0.1 --port 8003
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| POST | `/connect` | Connect to WebSocket URL |
| POST | `/disconnect` | Disconnect |
| GET | `/status` | Get connection status |
| POST | `/send` | Send message |
| GET | `/messages` | Get buffered messages |
| POST | `/reconnect` | Configure auto-reconnect |
| POST | `/ping` | Set ping interval |
| GET | `/events/stream` | SSE event stream |
