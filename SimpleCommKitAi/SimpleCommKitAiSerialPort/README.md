# SimpleCommKitAiSerialPort

An AI-friendly Serial Port toolkit powered by **simple_comm_kit_serialport**.
Enumerate, open, read, write, and monitor serial ports from AI agents and scripts.

## Key Features

- **MCP Server**: Expose serial port operations as tools for MCP-capable clients (Cursor, Claude Code, Windsurf, etc.)
- **HTTP Server**: Control serial ports over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiSerialPort
```

Note: This package depends on `simple_comm_kit_serialport` (SimpleCommKitPySerialPort) which must be built from source first.

## MCP Server

Expose serial port operations as tools for MCP-capable clients.

Configure it in your MCP client with:
```json
{
  "command": "simplecommkitaiserialport-mcp"
}
```

## HTTP Server

Run the REST API for controlling serial ports remotely:

```bash
simplecommkitaiserialport-http --host 127.0.0.1 --port 8001
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| GET | `/ports` | List available serial ports |
| POST | `/open/{port_name}` | Open a serial port |
| POST | `/close/{port_name}` | Close a serial port |
| GET | `/port/{port_name}` | Get port status & config |
| POST | `/port/{port_name}/read` | Read data from port |
| POST | `/port/{port_name}/write` | Write data to port |
| POST | `/port/{port_name}/config` | Update port configuration |
| POST | `/port/{port_name}/flush` | Flush port buffers |
