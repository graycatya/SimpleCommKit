---
name: simple-comm-kit-ai-hid-fastmcpp
description: Use the SimpleCommKitAiHidFastmcpp MCP server to enumerate, open, and interact with HID (Human Interface Device) devices. This skill provides guidance on the recommended flow (enumerate -> open -> write/read) and handles platform-specific differences like device paths on different OSes. Use when the user wants to interact with HID hardware or debug USB HID connections.
---

# SimpleCommKitAiHidFastmcpp (C++)

SimpleCommKitAiHidFastmcpp is a native C++ AI-friendly HID toolkit powered by the SimpleCommKitHid C++ library and the fastmcpp MCP framework. This skill provides instructions for using the SimpleCommKitAiHidFastmcpp MCP server to interact with HID (Human Interface Device) devices directly from the host machine.

## Quick Start Flow

Always follow this sequence for HID interactions:

1. **Enumerate**: Call `get_available_devices` to list available HID devices. Optionally filter by VID/PID.
2. **Open**: Call `open_device` using the `path` from the device list, or by VID/PID.
3. **Interaction**: Use `write` to send output reports, `send_feature_report` for feature reports, and `get_read_data` to retrieve buffered input reports.
4. **Cleanup**: Always call `close_device` when finished to release the device.

## Core Instructions

- **Device Paths**: Device paths are platform-specific. On Linux they look like `/dev/hidraw0`, on macOS they look like `IOService:...`, on Windows they look like `\\?\HID#...`. Always use the `path` field from `get_available_devices` results.
- **Open Modes**: By default, `open_device` opens devices in read+write mode. Set `readable=false` for write-only mode (e.g., for output-only devices like some keyboards/LEDs).
- **Data Handling**: Binary data is sent and received as hex strings. Input data is returned as `data_hex` (always reliable) and `data_utf8` (convenience field). If the data is not valid UTF-8, invalid bytes are skipped, so `data_utf8` may be incomplete or empty.
- **Read Buffering**: When `open_device` is called with `readable=true`, a background read thread is started automatically. Use `get_read_data` to retrieve buffered input reports.
- **Hotplug Monitoring**: Use `start_hotplug` to detect device connect/disconnect events (polling-based). Call `stop_hotplug` when done. Use `get_device_list` to see currently connected devices.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `get_available_devices` | List available HID devices | `vendor_id`, `product_id` (optional filters) |
| `open_device` | Open a HID device | `path` or `vendor_id`+`product_id`, `serial_number`, `readable` |
| `close_device` | Close a HID device | `path` (optional, closes all if omitted) |
| `write` | Write a report to a device | `path`, `data` (hex string) |
| `send_feature_report` | Send a feature report | `path`, `data` (hex string) |
| `start_hotplug` | Start hotplug detection | `vendor_id`, `product_id`, `poll_interval_ms` |
| `stop_hotplug` | Stop hotplug detection | None |
| `get_device_list` | Get cached device list | None |
| `get_open_paths` | List currently open devices | None |
| `get_read_data` | Retrieve buffered read data | `path` |
| `set_read_config` | Configure read polling | `path`, `poll_interval_ms`, `data_length` |

## Additional Resources

- For detailed tool documentation and platform notes, see [the reference guide](references/REFERENCE.md).
- For concrete usage examples, see [examples.md](references/examples.md).
- For troubleshooting common issues, see [troubleshooting.md](references/troubleshooting.md).
