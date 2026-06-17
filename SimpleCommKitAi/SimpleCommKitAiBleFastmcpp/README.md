# SimpleCommKitAiBleFastmcpp

Native C++ MCP server for BLE (Bluetooth Low Energy) device control, built on [fastmcpp](https://github.com/nicholasgriffintn/fastmcpp) and [SimpleCommKitBle](../src/SimpleCommKitBle).

## Overview

`SimpleCommKitAiBleFastmcpp` is the C++ counterpart of `SimpleCommKitAiBle`. It exposes **16 MCP tools** for AI agents to scan, connect, read, write, notify, and manage BLE devices — with native C++ performance and zero Python dependency.

## Architecture

```
AI Agent (MCP Client)
       │
       ▼ stdio / sse / streamable-http
┌──────────────────────────────────┐
│  SimpleCommKitAiBleFastmcpp      │  (C++ MCP Server)
├──────────────────────────────────┤
│  fastmcpp (C++ MCP framework)    │
├──────────────────────────────────┤
│  SimpleCommKitBle (C++ BLE lib)  │
├──────────────────────────────────┤
│  SimpleBLE (cross-platform BLE)  │
└──────────────────────────────────┘
```

## Building

### Prerequisites

- CMake 3.19+
- C++17 compiler
- The parent project (`SimpleCommKit`) must be configured first, which builds `SimpleCommKitBle` and `fastmcpp`

### Build

```bash
cd SimpleCommKit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target simplecommkitaible-fastmcpp
```

### Build with Tests

```bash
cmake .. -DSIMPLECOMMKITAIBLEFASTMCPP_BUILD_TESTS=ON
cmake --build . --target simplecommkitaible-fastmcpp ble_utility_test
ctest
```

## Usage

### MCP Client Configuration

Add to your MCP client (Claude Desktop, Cursor, etc.):

```json
{
  "mcpServers": {
    "simplecommkitaible-fastmcpp": {
      "command": "simplecommkitaible-fastmcpp"
    }
  }
}
```

### Command Line

```bash
# stdio mode (default) — used by MCP clients
simplecommkitaible-fastmcpp

# SSE mode — recommended for remote access
simplecommkitaible-fastmcpp --transport sse --host 127.0.0.1 --port 8003

# Streamable HTTP mode
simplecommkitaible-fastmcpp --transport streamable-http --port 8003

# Simple HTTP mode
simplecommkitaible-fastmcpp --transport http --port 8003
```

## Available Tools

| Tool | Description |
|------|-------------|
| `bluetooth_enabled` | Check if Bluetooth is enabled |
| `get_adapters` | List available Bluetooth adapters |
| `scan_for` | Scan for nearby BLE peripherals |
| `connect` | Connect to a peripheral |
| `disconnect` | Disconnect from a peripheral |
| `connected_devices` | List currently connected peripherals |
| `services` | List GATT services and characteristics |
| `read` | Read a characteristic value |
| `write_request` | Write with response (hex data) |
| `write_command` | Write without response (hex data) |
| `notify` | Subscribe to notifications |
| `indicate` | Subscribe to indications |
| `get_notifications` | Retrieve buffered notification data |
| `unsubscribe` | Unsubscribe from notifications |
| `descriptor_read` | Read a descriptor value |
| `descriptor_write` | Write to a descriptor (hex data) |

## Quick Start Flow

1. `get_adapters` → verify Bluetooth adapter is available
2. `scan_for` → discover nearby peripherals
3. `connect` → connect using the `address` from scan results
4. `services` → explore GATT services and characteristics
5. `read` / `write_request` / `write_command` → interact with characteristics
6. `notify` / `indicate` + `get_notifications` → stream data
7. `disconnect` → clean up when done

## Platform Support

- **Windows**: Uses WinRT BLE API
- **macOS**: Uses CoreBluetooth
- **Linux**: Uses BlueZ (requires `bluetoothd`)

## License

Apache 2.0 — See [LICENSE](../../LICENSE)
