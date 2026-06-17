---
name: simple-comm-kit-ai-usb-fastmcpp
description: Use the SimpleCommKitAiUsbFastmcpp MCP server to enumerate, open, and interact with USB devices via native C++. This skill provides guidance on the recommended flow (enumerate -> open -> claim interface -> transfer) and covers control, bulk, interrupt, and isochronous transfers. Use when the user wants to interact with USB hardware or debug USB connections through the native C++ server.
---

# SimpleCommKitAiUsbFastmcpp (C++)

SimpleCommKitAiUsbFastmcpp is a native C++ AI-friendly USB toolkit powered by the SimpleCommKitUsb C++ library and the fastmcpp MCP framework. This skill provides instructions for using the SimpleCommKitAiUsbFastmcpp MCP server to interact with USB devices directly from the host machine.

## Quick Start Flow

Always follow this sequence for USB interactions:

1. **Enumerate**: Call `get_available_devices` to discover connected USB devices. Use optional `vendor_id` and `product_id` filters.
2. **Open**: Call `open` using the `path` (e.g. `"1:3"`) from the enumeration results, or use `vendor_id` + `product_id`.
3. **Inspect**: Call `get_device_interfaces` and `get_interface_endpoints` to explore device capabilities.
4. **Claim**: Call `claim_interface` on the interface you want to use.
5. **Discover Endpoints**: Use `auto_discover_endpoints` or `find_endpoints_by_type` to find suitable IN/OUT endpoints.
6. **Transfer**: Use `control_transfer`, `bulk_transfer`, `interrupt_transfer`, or `isochronous_transfer` to communicate.
7. **Continuous Read**: Use `start_read_poll` + `get_read_data` for continuous data reception.
8. **Hotplug**: Use `start_hotplug` + `get_hotplug_data` to monitor device attach/detach events.
9. **Cleanup**: Call `release_interface` and `close` when finished.

## Core Instructions

- **Device Path**: The `path` field (e.g. `"1:3"` = bus:address) is the unique identifier for a USB device. Use it consistently across open, close, and transfer operations.
- **Endpoint Addresses**: IN endpoints have bit 7 set (e.g. `0x81`), OUT endpoints do not (e.g. `0x01`). Use `auto_discover_endpoints` to find them automatically.
- **Data Handling**: All write operations accept data as hex strings (e.g. `"00FF"` or `"01 02 AA"`). Spaces are automatically stripped. Read data is returned as both `data_hex` and `data_utf8`.
- **Interface Claiming**: You must claim an interface before performing any transfers on its endpoints. Release it when done.
- **Control Transfers**: Use `bmRequestType` to specify direction (bit 7 = 1 for IN, 0 for OUT) and recipient (bits 0-4). Common patterns: `0x80` (device-to-host, standard), `0x00` (host-to-device, standard).
- **Read Polling**: Data from `start_read_poll` is buffered and retrieved via `get_read_data`. Each call to `get_read_data` drains and clears the buffer.
- **Hotplug Events**: Hotplug events from all devices are collected into a global buffer. Use `get_hotplug_data` to retrieve them.

## MCP Tools Reference

| Tool | Description | Key Parameters |
|------|-------------|----------------|
| `get_available_devices` | Enumerate USB devices | `vendor_id`, `product_id` |
| `open` | Open a USB device | `path`, or `vendor_id`+`product_id`+`serial_number` |
| `close` | Close device(s) | `path` (omit = close all) |
| `is_open` | Check if device is open | `path` |
| `get_device_list` | Get cached/managed device list | `path` (optional) |
| `get_device_interfaces` | Get device interfaces | `path` |
| `get_interface_endpoints` | Get interface endpoints | `path`, `interface_number` |
| `find_endpoints_by_type` | Find endpoints by type | `path`, `transfer_type` |
| `auto_discover_endpoints` | Auto-discover IN/OUT endpoints | `path`, `transfer_type` |
| `claim_interface` | Claim an interface | `path`, `interface_number` |
| `release_interface` | Release an interface | `path`, `interface_number` |
| `control_transfer` | USB control transfer | `path`, `bmRequestType`, `bRequest`, `wValue`, `wIndex`, `data`, `timeout` |
| `bulk_transfer` | USB bulk transfer | `path`, `endpoint`, `data`/`read_length`, `timeout` |
| `interrupt_transfer` | USB interrupt transfer | `path`, `endpoint`, `data`/`read_length`, `timeout` |
| `isochronous_transfer` | USB isochronous transfer | `path`, `endpoint`, `num_packets`, `packet_length(s)`, `data`, `timeout` |
| `start_read_poll` | Start read polling | `path`, `endpoint` |
| `stop_read_poll` | Stop read polling | `path` |
| `get_read_data` | Get buffered read data | `path` |
| `start_hotplug` | Start hotplug detection | `path`, `vendor_id`, `product_id` |
| `stop_hotplug` | Stop hotplug detection | `path` |
| `get_hotplug_data` | Get hotplug events | None |
| `set_read_poll_interval` | Configure read poll | `path`, `ms`, `length` |
| `get_error` | Get device status | `path` |

## Common Transfer Scenarios

### Reading a Device Descriptor (Control Transfer)
```
control_transfer(path="1:3", bmRequestType=0x80, bRequest=0x06, wValue=0x0100, wIndex=0x0000, data_length=18)
```

### Reading from a Bulk Endpoint
```
auto_discover_endpoints(path="1:3", transfer_type="bulk")
claim_interface(path="1:3", interface_number=0)
bulk_transfer(path="1:3", endpoint=0x81, read_length=64)
```

### Writing to a Bulk Endpoint
```
bulk_transfer(path="1:3", endpoint=0x01, data="01020304")
```

### Continuous HID Interrupt Read
```
start_read_poll(path="1:3", endpoint=0x81)
# ... wait for data ...
get_read_data(path="1:3")
stop_read_poll(path="1:3")
```

## Platform Support

- **Windows**: Uses WinUSB/libusb-win32 via libusb. May require driver installation (Zadig).
- **macOS**: Uses IOKit via libusb.
- **Linux**: Uses usbfs via libusb. May require udev rules for device permissions.
