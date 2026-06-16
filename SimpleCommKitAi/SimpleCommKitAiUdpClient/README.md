# SimpleCommKitAiUdpClient

An AI-friendly UDP client toolkit powered by **SimpleCommKitPyUdp**.
Open a UDP socket, send datagrams, and receive data from AI agents and scripts.

## Key Features

- **MCP Server**: Expose UDP client operations as MCP tools
- **HTTP Server**: Control UDP client over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiUdpClient
```

## MCP Server

```json
{ "command": "simplecommkitaiudpclient-mcp" }
```

## HTTP Server

```bash
simplecommkitaiudpclient-http --host 127.0.0.1 --port 8002
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| POST | `/open` | Open UDP socket |
| POST | `/close` | Close UDP socket |
| GET | `/status` | Get socket status |
| POST | `/send` | Send datagram |
| GET | `/messages` | Get buffered messages |
| GET | `/events/stream` | SSE event stream |
