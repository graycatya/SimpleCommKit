# SimpleCommKitAiUsb

AI-friendly USB toolkit powered by **SimpleCommKitPyUsb** (libusb).

Provides USB operations as an MCP server and REST API for AI agents and scripts.

## Architecture

```
AI Agent (Cursor / Claude Code)
    │ MCP protocol (stdio / SSE / HTTP)
    ▼
SimpleCommKitAiUsb (MCP Server / HTTP API)
    │ Python
    ▼
SimpleCommKitPyUsb (pybind11 bindings)
    │ C++
    ▼
SimpleCommKitUsb (libusb wrapper)
    │ C
    ▼
libusb
```

## Components

| Component | Description |
|---|---|
| **MCP Server** | Exposes USB tools for AI agents (10+ tools) |
| **HTTP API** | RESTful HTTP endpoints for remote control |
| **SSE Stream** | Server-Sent Events for real-time read data |

## Usage

### MCP Server

```bash
# Stdio transport (for Cursor / Claude Code)
uv run simplecommkitaiusb-mcp --transport stdio

# HTTP transport
uv run simplecommkitaiusb-mcp --transport http --host 0.0.0.0 --port 8000
```

### HTTP API Server

```bash
uv run simplecommkitaiusb-http --host 0.0.0.0 --port 8000

# List devices
curl http://localhost:8000/devices

# Open device
curl -X POST http://localhost:8000/open -H "Content-Type: application/json" \
  -d '{"vid": "0x1234", "pid": "0x5678"}'

# Bulk transfer
curl -X POST http://localhost:8000/device/1:3/bulk_out \
  -H "Content-Type: application/json" \
  -d '{"endpoint": "0x01", "data": "AB CD EF"}'
```

## Installation

```bash
# Requires SimpleCommKitPyUsb to be built first
pip install -e .
```
