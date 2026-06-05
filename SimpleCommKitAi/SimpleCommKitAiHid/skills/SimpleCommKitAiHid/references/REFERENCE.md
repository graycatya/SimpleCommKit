# SimpleCommKitAiHid - Reference Guide

## Architecture

```
AI Agent (Cursor / Claude Code / Windsurf)
       │
       ▼ MCP Protocol (stdio or HTTP)
┌──────────────────────────┐
│  SimpleCommKitAiHid      │  ← This package
│  (MCP Server / REST API)  │
├──────────────────────────┤
│  SimpleCommKitPyHid      │  ← Python bindings (pybind11)
│  (simple_comm_kit_hid)    │
├──────────────────────────┤
│  SimpleCommKitHid        │  ← C++ HID abstraction
├──────────────────────────┤
│  hidapi                  │  ← Cross-platform HID
│  (libusb/hidraw/IOKit)   │
└──────────────────────────┘
```

## MCP Server Usage

The MCP server can be configured in any MCP-compatible AI client:

```json
{
  "mcpServers": {
    "simplecommkitaihid": {
      "command": "simplecommkitaihid-mcp"
    }
  }
}
```

### Transport Modes

- **stdio** (default): Standard input/output, used by most MCP clients
- **http**: HTTP server mode
- **streamable-http**: Recommended for remote access

```bash
# stdio mode (default)
simplecommkitaihid-mcp

# HTTP mode
simplecommkitaihid-mcp --transport http --host 0.0.0.0 --port 8002
```

## HTTP Server Usage

```bash
simplecommkitaihid-http --host 127.0.0.1 --port 8002
```

### Complete API Reference

#### GET /devices
Returns all available HID devices. Supports `?vendor_id=0x1234&product_id=0x5678` filter.

#### POST /open/{path}
Opens a HID device by platform path. Request body:
```json
{"path": "/dev/hidraw0", "readable": true}
```

#### POST /open
Opens a HID device by VID/PID. Request body:
```json
{"vendor_id": 4660, "product_id": 22136, "serial_number": "", "readable": true}
```

#### POST /close/{path} or POST /close
Closes a specific device or all devices.

#### POST /device/{path}/write
```json
{"data": "aabbccdd"}
```
Writes a report to the device. Data is hex string.

#### POST /device/{path}/feature
```json
{"data": "0501"}
```
Sends a feature report to the device.

#### GET /open_paths
Returns list of currently open device paths.

#### POST /hotplug/start
```json
{"vendor_id": 0, "product_id": 0, "poll_interval_ms": 1000}
```
Starts polling-based hotplug detection.

#### POST /hotplug/stop
Stops hotplug detection.

#### GET /hotplug/status
Returns hotplug status and configuration.

#### GET /device/{path}/config
Returns read poll configuration for a device.

#### POST /device/{path}/config
```json
{"read_poll_interval_ms": 100, "read_data_length": 64}
```
Updates read poll configuration.

#### GET /device/{path}/stream
SSE streaming endpoint for real-time read data.

## Platform Notes

### Linux
- Device paths: `/dev/hidraw0`, `/dev/hidraw1`, etc.
- Uses hidraw backend
- May require root or udev rules for access: `SUBSYSTEM=="hidraw", MODE="0666"`

### macOS
- Device paths: IOKit registry paths
- Uses IOHIDManager backend
- No special permissions needed for HID access

### Windows
- Device paths: `\\?\HID#VID_1234&PID_5678#...`
- Uses Windows HID API
- No special permissions needed for HID access
