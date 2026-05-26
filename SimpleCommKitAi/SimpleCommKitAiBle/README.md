# SimpleCommKitAiBle

An AI-friendly BLE toolkit powered by **SimpleCommKitPyBle**.
Scan, connect, and interact with Bluetooth Low Energy devices from AI agents and scripts.

## Key Features

- **MCP Server**: Expose BLE operations as tools for MCP-capable clients (Cursor, Claude Code, Windsurf, etc.)
- **HTTP Server**: Control BLE devices over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiBle
```

Note: This package depends on `SimpleCommKitPyBle` (SimpleCommKitPyBle) which must be built from source first.

## MCP Server

Expose BLE operations as tools for MCP-capable clients.

Configure it in your MCP client with:
```json
{
  "command": "simplecommkitaible-mcp"
}
```

## HTTP Server

Run the REST API for controlling BLE devices remotely:

```bash
simplecommkitaible-http --host 127.0.0.1 --port 8000
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| GET | `/adapters` | List available adapters |
| POST | `/scan` | Scan for peripherals |
| POST | `/connect/{address}` | Connect to a peripheral |
| POST | `/disconnect/{address}` | Disconnect from a peripheral |
| GET | `/device/{address}` | Get device info & services |
| POST | `/device/{address}/read/{svc}/{ch}` | Read characteristic |
| POST | `/device/{address}/write/{svc}/{ch}` | Write characteristic (with response) |
| POST | `/device/{address}/write_command/{svc}/{ch}` | Write characteristic (no response) |
| POST | `/device/{address}/notify/{svc}/{ch}` | Subscribe to notifications |
| POST | `/device/{address}/indicate/{svc}/{ch}` | Subscribe to indications |
| POST | `/device/{address}/unsubscribe/{svc}/{ch}` | Unsubscribe |
| GET | `/device/{address}/notifications` | Get buffered notifications |
