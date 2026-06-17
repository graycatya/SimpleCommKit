# SimpleCommKitAiUsbFastmcpp

Native C++ MCP server for USB device control, built on [fastmcpp](https://github.com/nicholasgriffintn/fastmcpp) and [SimpleCommKitUsb](../src/SimpleCommKitUsb).

## Overview

`SimpleCommKitAiUsbFastmcpp` exposes **23 MCP tools** for AI agents to enumerate, open, control, and monitor USB devices — with native C++ performance and zero Python dependency.

## Architecture

```
AI Agent (MCP Client)
       │
       ▼ stdio / sse / streamable-http
┌──────────────────────────────────┐
│  SimpleCommKitAiUsbFastmcpp      │  (C++ MCP Server)
├──────────────────────────────────┤
│  fastmcpp (C++ MCP framework)    │
├──────────────────────────────────┤
│  SimpleCommKitUsb (C++ USB lib)  │
├──────────────────────────────────┤
│  libusb-1.0 (cross-platform USB) │
└──────────────────────────────────┘
```

## Building

### Prerequisites

- CMake 3.19+
- C++17 compiler
- libusb-1.0
- The parent project (`SimpleCommKit`) must be configured first, which builds `SimpleCommKitUsb` and `fastmcpp`

### Build

```bash
cd SimpleCommKit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target simplecommkitaiusb-fastmcpp
```

### Build with Tests

```bash
cmake .. -DSIMPLECOMMKITAIUSBFASTMCPP_BUILD_TESTS=ON
cmake --build . --target simplecommkitaiusb-fastmcpp usb_utility_test
ctest
```

## Usage

### MCP Client Configuration

Add to your MCP client (Claude Desktop, Cursor, etc.):

```json
{
  "mcpServers": {
    "simplecommkitaiusb-fastmcpp": {
      "command": "simplecommkitaiusb-fastmcpp"
    }
  }
}
```

### Command Line

```bash
# stdio mode (default) — used by MCP clients
simplecommkitaiusb-fastmcpp

# SSE mode — recommended for remote access
simplecommkitaiusb-fastmcpp --transport sse --host 127.0.0.1 --port 8007

# Streamable HTTP mode
simplecommkitaiusb-fastmcpp --transport streamable-http --port 8007

# Simple HTTP mode
simplecommkitaiusb-fastmcpp --transport http --port 8007
```

## Available Tools

| Tool | Description |
|------|-------------|
| `get_available_devices` | Enumerate USB devices (optionally filter by VID/PID) |
| `open` | Open a USB device by path or VID/PID/serial |
| `close` | Close a USB device |
| `is_open` | Check if a device is open |
| `get_device_list` | Get cached device list |
| `get_device_interfaces` | Get interfaces of the opened device |
| `get_interface_endpoints` | Get endpoints for a specific interface |
| `find_endpoints_by_type` | Find endpoints by transfer type (bulk/interrupt/isoch) |
| `auto_discover_endpoints` | Auto-discover IN/OUT endpoints for a transfer type |
| `claim_interface` | Claim a USB interface |
| `release_interface` | Release a USB interface |
| `control_transfer` | Perform a USB control transfer |
| `bulk_transfer` | Read/write via bulk endpoint |
| `interrupt_transfer` | Read/write via interrupt endpoint |
| `isochronous_transfer` | Read/write via isochronous endpoint |
| `start_read_poll` | Start continuous read polling on an endpoint |
| `stop_read_poll` | Stop read polling |
| `get_read_data` | Retrieve buffered read data |
| `start_hotplug` | Start hotplug detection (polling-based) |
| `stop_hotplug` | Stop hotplug detection |
| `get_hotplug_data` | Retrieve hotplug events (added/removed devices) |
| `set_read_poll_interval` | Configure read poll interval |
| `get_error` | Get and clear the last error |

## Quick Start Flow

1. `get_available_devices` → discover connected USB devices
2. `open` → open a device by its `path` (e.g. `"1:3"`)
3. `claim_interface` → claim the interface you want to use
4. `get_device_interfaces` / `get_interface_endpoints` → inspect device capabilities
5. `control_transfer` / `bulk_transfer` / `interrupt_transfer` → communicate with the device
6. `start_read_poll` + `get_read_data` → continuously read incoming data
7. `release_interface` + `close` → clean up when done

## Platform Support

- **Windows**: Uses WinUSB/libusb-win32 via libusb
- **macOS**: Uses IOKit via libusb
- **Linux**: Uses usbfs via libusb

## License

Apache 2.0 — See [LICENSE](../../LICENSE)
