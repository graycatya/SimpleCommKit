---
name: simple-comm-kit-ai-ble-fastmcpp
description: Use the SimpleCommKitAiBleFastmcpp MCP server to scan, connect, and interact with Bluetooth devices via native C++. This skill provides guidance on the recommended flow (scan -> connect -> services -> read/notify) and handles platform-specific differences like UUIDs on macOS vs MAC addresses on Linux. Use when the user wants to interact with BLE hardware or debug Bluetooth connections through the native C++ server.
---

# SimpleCommKitAiBleFastmcpp (C++)

SimpleCommKitAiBleFastmcpp is a native C++ AI-friendly BLE toolkit powered by the SimpleCommKitBle C++ library and the fastmcpp MCP framework. This skill provides instructions for using the SimpleCommKitAiBleFastmcpp MCP server to interact with Bluetooth Low Energy (BLE) devices directly from the host machine.

## Quick Start Flow

Always follow this sequence for BLE interactions:

1. **Adapters**: Call `get_adapters` to verify a Bluetooth adapter is available.
2. **Scanning**: Call `scan_for` (default 5s) to find nearby peripherals.
3. **Connection**: Call `connect` using the `address` from the scan results.
4. **Exploration**: Call `services` to list available GATT services and characteristics.
5. **Interaction**: Use `read` for one-time values, `write_request`/`write_command` to send data, or `notify`/`indicate` + `get_notifications` + `unsubscribe` for streaming data.
6. **Cleanup**: Always call `disconnect` when finished to release the device.

## Core Instructions

- **Scanning**: Prefer scanning immediately before connecting to ensure the device is in the internal cache.
- **Addressing**: Be aware that macOS/iOS uses UUIDs for addresses, while Linux/Windows uses MAC addresses.
- **Data Handling**: Binary data is returned as `data_hex` (always reliable) and `data_utf8` (convenience field). If the data is not valid UTF-8, invalid bytes are skipped, so `data_utf8` may be incomplete or empty. Use `data_hex` for protocol analysis and `data_utf8` for human-readable strings.
- **Write Data**: All write operations accept data as hex strings (e.g. `"00FF"` or `"01 02 AA"`). Spaces are automatically stripped.
- **Notifications/Indications**: Use `notify` or `indicate` to subscribe, `get_notifications` to retrieve buffered data, and `unsubscribe` when done. Data is buffered per-address.
- **Bluetooth Status**: Assume Bluetooth is enabled by default. Only check `bluetooth_enabled` when an operation fails.
- **UUIDs**: Service and characteristic UUIDs can be 16-bit (e.g. `"180a"`), 32-bit, or 128-bit (e.g. `"0000180a-0000-1000-8000-00805f9b34fb"`). Use the format returned by the `services` tool.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `bluetooth_enabled` | Check if Bluetooth is enabled | None |
| `get_adapters` | List available adapters | None |
| `scan_for` | Scan for peripherals | `timeout_ms` (default: 5000) |
| `connect` | Connect to a peripheral | `address` |
| `disconnect` | Disconnect from a peripheral | `address` |
| `connected_devices` | List currently connected peripherals | None |
| `services` | List GATT services & characteristics | `address` |
| `read` | Read a characteristic value | `address`, `service_uuid`, `characteristic_uuid` |
| `write_request` | Write with response (hex data) | `address`, `service_uuid`, `characteristic_uuid`, `data` |
| `write_command` | Write without response (hex data) | `address`, `service_uuid`, `characteristic_uuid`, `data` |
| `notify` | Subscribe to notifications | `address`, `service_uuid`, `characteristic_uuid` |
| `indicate` | Subscribe to indications | `address`, `service_uuid`, `characteristic_uuid` |
| `get_notifications` | Retrieve buffered notification data | `address` |
| `unsubscribe` | Unsubscribe from notifications | `address`, `service_uuid`, `characteristic_uuid` |
| `descriptor_read` | Read a descriptor value | `address`, `service_uuid`, `characteristic_uuid`, `descriptor_uuid` |
| `descriptor_write` | Write to a descriptor (hex data) | `address`, `service_uuid`, `characteristic_uuid`, `descriptor_uuid`, `data` |

## Additional Resources

- For detailed tool documentation and platform notes, see [the reference guide](references/REFERENCE.md).
- For concrete usage examples, see [examples.md](references/examples.md).
- For troubleshooting common issues, see [troubleshooting.md](references/troubleshooting.md).
