# SimpleCommKitAiTcpClient

An AI-friendly TCP client toolkit powered by **SimpleCommKitPyTcp**.
Connect, send, and receive data over TCP from AI agents and scripts.

## Key Features

- **MCP Server**: Expose TCP client operations as tools for MCP-capable clients (Cursor, Claude Code, Windsurf, etc.)
- **HTTP Server**: Control TCP client connections over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiTcpClient
```

Note: This package depends on `SimpleCommKitPyTcp` which must be built from source first.

## MCP Server

Configure it in your MCP client with:
```json
{
  "command": "simplecommkitaictcpclient-mcp"
}
```

## HTTP Server

```bash
simplecommkitaictcpclient-http --host 127.0.0.1 --port 8001
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| POST | `/connect` | Connect to TCP server |
| POST | `/disconnect` | Disconnect from server |
| GET | `/status` | Get connection status |
| POST | `/send` | Send data |
| GET | `/messages` | Get buffered messages |
| GET | `/events/stream` | SSE real-time event stream |
