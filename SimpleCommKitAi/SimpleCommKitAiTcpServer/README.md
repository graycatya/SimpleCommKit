# SimpleCommKitAiTcpServer

An AI-friendly TCP server toolkit powered by **SimpleCommKitPyTcp**.
Start a TCP server, accept clients, and send/receive data from AI agents and scripts.

## Key Features

- **MCP Server**: Expose TCP server operations as tools for MCP-capable clients
- **HTTP Server**: Control TCP server over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiTcpServer
```

## MCP Server

Configure it in your MCP client with:
```json
{
  "command": "simplecommkitaictcpserver-mcp"
}
```

## HTTP Server

```bash
simplecommkitaictcpserver-http --host 127.0.0.1 --port 8101
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| POST | `/start` | Start TCP server |
| POST | `/stop` | Stop TCP server |
| GET | `/status` | Get server status |
| GET | `/clients` | List connected clients |
| POST | `/send/{client_id}` | Send to a client |
| POST | `/broadcast` | Broadcast to all clients |
| GET | `/messages` | Get buffered messages |
| GET | `/events/stream` | SSE real-time event stream |
