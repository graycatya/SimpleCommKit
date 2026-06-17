# Reference Guide

## Platform-Specific Behavior

### macOS (CoreBluetooth)

- Device **addresses are UUIDs** (not MAC addresses). They look like `"A1B2C3D4-1234-5678-9ABC-DEF012345678"`.
- Scanning requires user permission. The app must be granted Bluetooth access.
- Multiple adapters are rare on macOS (usually one built-in).

### Linux (BlueZ)

- Device **addresses are MAC addresses**. They look like `"AA:BB:CC:DD:EE:FF"`.
- Requires `bluetoothd` service to be running.
- May need root permissions or proper udev rules for full BLE access.
- `get_adapters` typically returns `hci0`.

### Windows (WinRT)

- Device **addresses are MAC addresses**. They look like `"aabbccddeeff"` (lowercase, no separators).
- Requires Bluetooth adapter to be enabled in Windows Settings.
- Windows 10 build 15063+ is required for full BLE support.

## Data Format

### Hex Strings

All write operations (`write_request`, `write_command`, `descriptor_write`) accept data as hex strings. Accepted formats:

- Continuous: `"00FFAB12"`
- Spaced: `"00 FF AB 12"`
- Mixed case: `"00ffAb12"`

### Read Results

Read operations return:
- `data_hex`: Hex string of raw bytes (always available)
- `data_utf8`: Best-effort UTF-8 interpretation (may be truncated)
- `data_length`: Number of bytes

### Notification Data

Notifications are buffered per-address. Each entry includes:
- `address`: Device address
- `service_uuid`: Source service
- `characteristic_uuid`: Source characteristic
- `data_hex`: Payload as hex
- `data_utf8`: Payload as UTF-8 (best-effort)
- `data_length`: Payload size in bytes

## Common BLE UUIDs

| Service | UUID |
|---------|------|
| Device Information | `180a` |
| Battery Service | `180f` |
| Heart Rate | `180d` |
| Environmental Sensing | `181a` |
| Generic Access | `1800` |
| Generic Attribute | `1801` |

| Characteristic | UUID |
|----------------|------|
| Device Name | `2a00` |
| Appearance | `2a01` |
| Manufacturer Name | `2a29` |
| Model Number | `2a24` |
| Serial Number | `2a25` |
| Firmware Revision | `2a26` |
| Battery Level | `2a19` |
| Heart Rate Measurement | `2a37` |
| Temperature | `2a6e` |

## Transport Modes

### stdio (default)

Used by MCP clients that spawn the server process. JSON-RPC messages are exchanged via stdin/stdout.

```json
{
  "mcpServers": {
    "simplecommkitaible-fastmcpp": {
      "command": "simplecommkitaible-fastmcpp"
    }
  }
}
```

### SSE (Server-Sent Events)

Recommended for remote or web-based access.

```json
{
  "mcpServers": {
    "simplecommkitaible-fastmcpp": {
      "url": "http://localhost:8003/sse"
    }
  }
}
```

### Streamable HTTP

Modern transport with streaming support.

```json
{
  "mcpServers": {
    "simplecommkitaible-fastmcpp": {
      "url": "http://localhost:8003/mcp"
    }
  }
}
```
