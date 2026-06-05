# SimpleCommKitAiHid

An AI-friendly HID toolkit powered by **SimpleCommKitPyHid**.
Enumerate, open, write, and monitor HID (Human Interface Device) devices from AI agents and scripts.

## Key Features

- **MCP Server**: Expose HID operations as tools for MCP-capable clients (Cursor, Claude Code, Windsurf, etc.)
- **HTTP Server**: Control HID devices over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiHid
```

Note: This package depends on `SimpleCommKitPyHid` (SimpleCommKitPyHid) which must be built from source first.

## MCP Server

Expose HID operations as tools for MCP-capable clients.

Configure it in your MCP client with:
```json
{
  "command": "simplecommkitaihid-mcp"
}
```

## HTTP Server

Run the REST API for controlling HID devices remotely:

```bash
simplecommkitaihid-http --host 127.0.0.1 --port 8002
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| GET | `/devices` | List available HID devices |
| POST | `/open/{path}` | Open a HID device by path |
| POST | `/open` | Open by VID/PID/serial |
| POST | `/close/{path}` | Close a specific device |
| POST | `/close` | Close all devices |
| POST | `/device/{path}/write` | Write a report to a device |
| POST | `/device/{path}/feature` | Send a feature report |
| POST | `/hotplug/start` | Start hotplug detection |
| POST | `/hotplug/stop` | Stop hotplug detection |
| GET | `/hotplug/status` | Get hotplug status |
