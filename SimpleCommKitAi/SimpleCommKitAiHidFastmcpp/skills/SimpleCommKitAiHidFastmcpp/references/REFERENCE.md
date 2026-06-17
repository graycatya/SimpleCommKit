# SimpleCommKitAiHidFastmcpp - Reference Guide

## Architecture

```
AI Agent (Cursor / Claude Code / Windsurf)
       │
       ▼ MCP Protocol (stdio / sse / streamable-http)
┌──────────────────────────────────┐
│  SimpleCommKitAiHidFastmcpp      │  ← This package (C++ native)
│  (MCP Server)                     │
├──────────────────────────────────┤
│  fastmcpp                         │  ← C++ MCP framework
├──────────────────────────────────┤
│  SimpleCommKitHid                 │  ← C++ HID abstraction
├──────────────────────────────────┤
│  hidapi                           │  ← Cross-platform HID
│  (libusb/hidraw/IOKit/WinHID)     │
└──────────────────────────────────┘
```

Unlike the Python variant (`SimpleCommKitAiHid`), this implementation is a **native C++ binary** that links directly to `SimpleCommKitHid` and `fastmcpp`, offering lower latency and zero Python dependency.

## MCP Server Usage

The MCP server can be configured in any MCP-compatible AI client:

```json
{
  "mcpServers": {
    "simplecommkitaihid-fastmcpp": {
      "command": "simplecommkitaihid-fastmcpp"
    }
  }
}
```

### Transport Modes

- **stdio** (default): Standard input/output, used by most MCP clients
- **sse**: Server-Sent Events transport (recommended for remote access)
- **streamable-http**: Streamable HTTP per MCP spec 2025-03-26
- **http**: Simple HTTP POST transport

```bash
# stdio mode (default)
simplecommkitaihid-fastmcpp

# SSE mode
simplecommkitaihid-fastmcpp --transport sse --host 127.0.0.1 --port 8002

# Streamable HTTP mode
simplecommkitaihid-fastmcpp --transport streamable-http --host 0.0.0.0 --port 8002

# HTTP mode
simplecommkitaihid-fastmcpp --transport http --port 8002
```

## MCP Tools Detail

### get_available_devices

List available HID devices, optionally filtered by VID/PID.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| vendor_id | integer | 0 | USB vendor ID filter (0 = all) |
| product_id | integer | 0 | USB product ID filter (0 = all) |

**Returns:** Array of device info objects, each with `path`, `manufacturer_string`, `product_string`, `serial_number`, `bus_type`, `bus_type_name`, `interface_number`, `release_number`.

### open_device

Open a HID device for communication.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| path | string | "" | Platform-specific device path (takes priority) |
| vendor_id | integer | 0 | USB vendor ID (used when path is empty) |
| product_id | integer | 0 | USB product ID (used when path is empty) |
| serial_number | string | "" | Optional serial number filter |
| readable | boolean | true | Enable read mode |

**Returns:** `{"message": "...", "path": "..."}`

### close_device

Close a HID device (or all devices).

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| path | string | "" | Device path (omit to close all) |

**Returns:** `{"message": "..."}`

### write

Write a report to an open HID device.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| path | string | (required) | Path of the open device |
| data | string | (required) | Hex string (e.g. "00FF" or "01 02 AA") |

**Returns:** `{"message": "...", "path": "...", "bytes_written": N}`

### send_feature_report

Send a HID feature report.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| path | string | (required) | Path of the open device |
| data | string | (required) | Hex string |

**Returns:** `{"message": "...", "path": "..."}`

### start_hotplug

Start polling-based hotplug detection.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| vendor_id | integer | 0 | VID filter (0 = all) |
| product_id | integer | 0 | PID filter (0 = all) |
| poll_interval_ms | integer | 1000 | Poll interval in ms |

**Returns:** `{"message": "...", "vendor_id": ..., "product_id": ..., "poll_interval_ms": ...}`

### stop_hotplug

Stop hotplug detection. No parameters.

**Returns:** `{"message": "Hotplug detection stopped"}`

### get_device_list

Get the cached device list (populated by `open_device` or `start_hotplug`). No parameters.

**Returns:** Array of device info objects.

### get_open_paths

Get list of currently open device paths and status. No parameters.

**Returns:** `{"open_paths": [...], "open_count": N, "hotplug_active": bool}`

### get_read_data

Retrieve and clear buffered read data.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| path | string | (required) | Path of the open device |

**Returns:** Array of `{"path": "...", "data_hex": "...", "data_utf8": "...", "data_length": N}`

### set_read_config

Configure read polling parameters.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| path | string | (required) | Path of the open device |
| poll_interval_ms | integer | 0 | Poll interval in ms (0 = no change) |
| data_length | integer | 0 | Bytes per poll (0 = no change) |

**Returns:** `{"message": "...", "path": "...", "read_poll_interval_ms": ..., "read_data_length": ...}`

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
