---
name: simple-comm-kit-ai-usb
description: Use the SimpleCommKitAiUsb MCP server to enumerate, open, and interact with USB devices. This skill provides guidance on the recommended flow (enumerate, open, claim, transfer) and common USB operations (control/bulk/interrupt transfers, hotplug monitoring).
---

## Overview

The `simple-comm-kit-ai-usb` skill provides USB device operations via MCP (Model Context Protocol).
It wraps **libusb** through **SimpleCommKitUsb** (C++) and **SimpleCommKitPyUsb** (pybind11),
enabling AI agents to enumerate, open, and communicate with USB devices.

## MCP Server Entry Point

Use `simplecommkitaiusb-mcp` to start the MCP server:

```bash
# Stdio transport (for MCP clients like Cursor/Claude)
simplecommkitaiusb-mcp --transport stdio

# HTTP transport (for remote use)
simplecommkitaiusb-mcp --transport http --host 0.0.0.0 --port 8003
```

## MCP Configuration (for AI Clients)

Add this to your MCP client configuration:

```json
{
  "mcpServers": {
    "simple-comm-kit-ai-usb": {
      "command": "simplecommkitaiusb-mcp",
      "args": ["--transport", "stdio"]
    }
  }
}
```

## Recommended Flow

1. **Enumerate**: `get_available_devices [vendor_id=0] [product_id=0]` — find target device
2. **Open**: `open_device path="1:3"` or `open_device vendor_id=0x1234 product_id=0x5678` — get a handle
3. **Claim Interface**: `claim_interface path="1:3" interface_number=0` — claim the USB interface
4. **Transfer Data**:
   - `control_transfer` — for standard requests (e.g. get descriptor)
   - `bulk_transfer_out` / `bulk_transfer_in` — for bulk data
   - `interrupt_transfer_out` / `interrupt_transfer_in` — for interrupt endpoints
5. **Continuous Read** (optional): `start_read_poll path endpoint` → `get_read_data path` → `stop_read_poll path`
6. **Close**: `close_device [path]`

## Hotplug

- The module auto-detects native libusb hotplug support (`LIBUSB_CAP_HAS_HOTPLUG`)
- With native support: **event-driven** callbacks (no polling)
- Without native support: **polling-based** detection (configurable interval)
- Use `start_hotplug` / `stop_hotplug` to control

## MCP Tools Reference

| Tool | Tags | Description |
|------|------|-------------|
| `get_available_devices` | discovery | Enumerate USB devices |
| `open_device` | connection | Open by path or VID/PID/serial |
| `close_device` | connection | Close device(s) |
| `claim_interface` | config | Claim a USB interface |
| `release_interface` | config | Release a USB interface |
| `control_transfer` | io | USB control transfer (IN/OUT) |
| `bulk_transfer_out` | io | Bulk OUT transfer |
| `bulk_transfer_in` | io | Bulk IN transfer |
| `interrupt_transfer_out` | io | Interrupt OUT transfer |
| `interrupt_transfer_in` | io | Interrupt IN transfer |
| `start_read_poll` | lifecycle | Start continuous endpoint polling |
| `stop_read_poll` | lifecycle | Stop continuous polling |
| `get_read_data` | io | Retrieve buffered read data |
| `start_hotplug` | monitor | Start hotplug monitoring |
| `stop_hotplug` | monitor | Stop hotplug monitoring |
| `get_device_list` | discovery | Get cached device list |
| `get_open_paths` | status | Get open device paths + status |
| `set_read_config` | config | Configure poll interval / data length |

## Device Path Format

USB devices are identified by `"bus:address"` format (e.g. `"1:3"` = bus 1, address 3).

## Platform Notes

- **Windows**: May need WinUSB or libusbK driver (use Zadig)
- **macOS**: Uses IOKit, generally works without extra setup
- **Linux**: May need udev rules for non-root access

See `references/troubleshooting.md` for common issues.
