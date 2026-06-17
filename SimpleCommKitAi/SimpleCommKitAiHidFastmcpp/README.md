# SimpleCommKitAiHidFastmcpp

Native C++ MCP server for HID (Human Interface Device) device control, built on [fastmcpp](https://github.com/nicholasgriffintn/fastmcpp) and [SimpleCommKitHid](../src/SimpleCommKitHid).

## Overview

`SimpleCommKitAiHidFastmcpp` is the C++ counterpart of `SimpleCommKitAiHid`. It exposes 11 MCP tools for AI agents to enumerate, open, read, write, and monitor HID devices — with native C++ performance and zero Python dependency.

## Architecture

```
AI Agent (MCP Client)
       │
       ▼ stdio / sse / streamable-http
┌──────────────────────────────────┐
│  SimpleCommKitAiHidFastmcpp      │  (C++ MCP Server)
├──────────────────────────────────┤
│  fastmcpp (C++ MCP framework)    │
├──────────────────────────────────┤
│  SimpleCommKitHid (C++ HID lib)  │
├──────────────────────────────────┤
│  hidapi (cross-platform HID)     │
└──────────────────────────────────┘
```

## Building

### Prerequisites

- CMake 3.19+
- C++17 compiler
- The parent project (`SimpleCommKit`) must be configured first, which builds `SimpleCommKitHid` and `fastmcpp`

### Build

```bash
cd SimpleCommKit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target simplecommkitaihid-fastmcpp
```

### Build with Tests

```bash
cmake .. -DSIMPLECOMMKITAIHIDFASTMCPP_BUILD_TESTS=ON
cmake --build . --target simplecommkitaihid-fastmcpp hid_utility_test hid_state_test
ctest
```

## Usage

### MCP Client Configuration

Add to your MCP client (Claude Desktop, Cursor, etc.):

```json
{
  "mcpServers": {
    "simplecommkitaihid-fastmcpp": {
      "command": "simplecommkitaihid-fastmcpp"
    }
  }
}
```

### Command Line

```bash
# stdio mode (default) — used by MCP clients
simplecommkitaihid-fastmcpp

# SSE mode — recommended for remote access
simplecommkitaihid-fastmcpp --transport sse --host 127.0.0.1 --port 8002

# Streamable HTTP mode
simplecommkitaihid-fastmcpp --transport streamable-http --port 8002

# Simple HTTP mode
simplecommkitaihid-fastmcpp --transport http --port 8002
```

## Available Tools

| Tool | Description |
|------|-------------|
| `get_available_devices` | List available HID devices with optional VID/PID filtering |
| `open_device` | Open a HID device by path or VID/PID/serial |
| `close_device` | Close a specific device or all devices |
| `write` | Write a hex report to a device |
| `send_feature_report` | Send a feature report to a device |
| `start_hotplug` | Start polling-based hotplug detection |
| `stop_hotplug` | Stop hotplug detection |
| `get_device_list` | Get cached device list |
| `get_open_paths` | List open device paths and status |
| `get_read_data` | Retrieve and clear buffered read data |
| `set_read_config` | Configure read polling parameters |

## Quick Start Flow

1. `get_available_devices` → find your device
2. `open_device` → open by path or VID/PID
3. `write` / `send_feature_report` → send data
4. `get_read_data` → retrieve buffered input
5. `close_device` → clean up

## Platform Support

- **Windows**: Uses Windows HID API (`hid.dll`)
- **macOS**: Uses IOHIDManager
- **Linux**: Uses hidraw backend (may require udev rules)

## License

Apache 2.0 — See [LICENSE](../../LICENSE)
