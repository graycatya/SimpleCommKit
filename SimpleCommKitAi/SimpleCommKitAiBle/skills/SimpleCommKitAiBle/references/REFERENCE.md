# SimpleCommKitAiBle - Reference Guide

## Architecture

```
AI Agent (Cursor / Claude Code / Windsurf)
       │
       ▼ MCP Protocol (stdio or HTTP)
┌──────────────────────────┐
│  SimpleCommKitAiBle      │  ← This package
│  (MCP Server / REST API)  │
├──────────────────────────┤
│  SimpleCommKitPyBle      │  ← Python bindings (pybind11)
│  (simple_comm_kit_ble)    │
├──────────────────────────┤
│  SimpleCommKitBle        │  ← C++ BLE abstraction
├──────────────────────────┤
│  SimpleBLE               │  ← Cross-platform BLE
│  WinRT / CoreBT / BlueZ  │
└──────────────────────────┘
```

## MCP Server Usage

The MCP server can be configured in any MCP-compatible AI client:

```json
{
  "mcpServers": {
    "simplecommkitaible": {
      "command": "simplecommkitaible-mcp"
    }
  }
}
```

### Transport Modes

- **stdio** (default): Standard input/output, used by most MCP clients
- **http**: HTTP server mode

```bash
# stdio mode (default)
simplecommkitaible-mcp

# HTTP mode
simplecommkitaible-mcp --transport http --host 0.0.0.0 --port 8000
```

## HTTP Server Usage

```bash
simplecommkitaible-http --host 127.0.0.1 --port 8000
```

### Complete API Reference

#### GET /adapters
Returns all available Bluetooth adapters.

#### POST /scan?timeout_ms=5000
Scans for BLE peripherals. Results are cached for `connect`.

#### POST /connect/{address}
Connects to a peripheral. Must be in the last scan results.

#### POST /disconnect/{address}
Disconnects from a peripheral.

#### GET /device/{address}
Returns device info: connected status, MTU, and list of GATT services with characteristics.

#### POST /device/{address}/read/{service_uuid}/{char_uuid}
Reads a characteristic value. Returns `data_hex` and `data_utf8`.

#### POST /device/{address}/write/{service_uuid}/{char_uuid}
```json
{"data": "aabbccdd"}
```
Writes hex data to a characteristic with response.

#### POST /device/{address}/write_command/{service_uuid}/{char_uuid}
Same as write but without waiting for a response.

#### POST /device/{address}/notify/{service_uuid}/{char_uuid}
Subscribes to notifications. Data is buffered internally.

#### POST /device/{address}/indicate/{service_uuid}/{char_uuid}
Subscribes to indications. Data is buffered internally.

#### GET /device/{address}/notifications
Returns and clears all buffered notification/indication data.

#### POST /device/{address}/unsubscribe/{service_uuid}/{char_uuid}
Unsubscribes from notifications/indications.

#### POST /device/{address}/read_descriptor/{service_uuid}/{char_uuid}/{desc_uuid}
Reads a descriptor value. Returns `data_hex` and `data_utf8`. Descriptors contain metadata (e.g., CCCD for enabling notifications).

#### POST /device/{address}/write_descriptor/{service_uuid}/{char_uuid}/{desc_uuid}
```json
{"data": "0100"}
```
Writes hex data to a descriptor (e.g., `0100` to enable notifications on a CCCD).

## Platform Notes

### macOS / iOS
- Peripheral addresses are UUIDs (not MAC addresses)
- Uses CoreBluetooth backend

### Linux
- Peripheral addresses are MAC addresses (e.g., "AA:BB:CC:DD:EE:FF")
- Uses BlueZ backend
- Requires `bluetoothd` running

### Windows
- Peripheral addresses are MAC addresses
- Uses WinRT backend
- Requires Bluetooth enabled in Windows settings
