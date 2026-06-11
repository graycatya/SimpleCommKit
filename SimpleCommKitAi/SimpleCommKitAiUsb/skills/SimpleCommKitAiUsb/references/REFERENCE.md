# SimpleCommKitAiUsb Reference

## Architecture

```
AI Agent → MCP/HTTP → SimpleCommKitAiUsb → SimpleCommKitPyUsb → SimpleCommKitUsb (C++) → libusb
```

## MCP Server Configuration

Add to your AI client's MCP configuration:

```json
{
  "mcpServers": {
    "simple-comm-kit-ai-usb": {
      "command": "python",
      "args": ["-m", "SimpleCommKitAiUsb.mcp", "--transport", "stdio"]
    }
  }
}
```

For HTTP transport:

```json
{
  "mcpServers": {
    "simple-comm-kit-ai-usb": {
      "command": "python",
      "args": ["-m", "SimpleCommKitAiUsb.mcp", "--transport", "http", "--host", "0.0.0.0", "--port", "8000"]
    }
  }
}
```

## Device Path Format

USB devices are identified by `"bus:address"` format:
- Bus number: integer from libusb enumeration
- Address: integer device address on that bus
- Example: `"1:3"` means bus 1, device address 3

## Endpoint Addresses

- **OUT endpoints**: 0x01, 0x02, etc. (bit 7 = 0)
- **IN endpoints**: 0x81, 0x82, etc. (bit 7 = 1)
- For `interrupt_transfer_in` and `bulk_transfer_in`, use IN endpoint addresses
- For `interrupt_transfer_out` and `bulk_transfer_out`, use OUT endpoint addresses

## HTTP API Reference

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/devices` | List USB devices (filter: `?vendor_id=0x1234&product_id=0x5678`) |
| POST | `/open/{path}` | Open device by path |
| POST | `/open` | Open by VID/PID (body: `{vid, pid, serial}`) |
| POST | `/close/{path}` | Close specific device |
| POST | `/close` | Close all devices |
| GET | `/open_paths` | List open device paths |
| POST | `/device/{path}/claim?interface_number=N` | Claim interface |
| POST | `/device/{path}/release?interface_number=N` | Release interface |
| POST | `/device/{path}/control` | Control transfer |
| POST | `/device/{path}/bulk_out` | Bulk OUT transfer |
| POST | `/device/{path}/bulk_in` | Bulk IN transfer |
| POST | `/device/{path}/intr_out` | Interrupt OUT transfer |
| POST | `/device/{path}/intr_in` | Interrupt IN transfer |
| POST | `/read/start` | Start read poll |
| POST | `/read/stop` | Stop read poll |
| GET | `/read/stream` | SSE read data stream |
| POST | `/hotplug/start` | Start hotplug |
| POST | `/hotplug/stop` | Stop hotplug |
| GET | `/hotplug/status` | Hotplug status |

## MCP Tools Reference

All tools accept and return dict/JSON. Endpoint and hex values use string hex format (e.g. `"0x81"`).

### get_available_devices
- **Inputs**: `vendor_id` (str hex), `product_id` (str hex)
- **Returns**: list of device info dicts

### open_device
- **Inputs**: `path` (str) OR `vendor_id` + `product_id` + optional `serial_number`
- **Returns**: `{success, path | vid+pid}`

### control_transfer
- **Inputs**: `path`, `bm_request_type`, `b_request`, `w_value`, `w_index`, `data_hex`, `length`, `timeout`
- **Returns**: `{success, length, data}` (hex string)

### bulk_transfer_out / bulk_transfer_in
- **Inputs**: `path`, `endpoint`, `data_hex` or `length`, `timeout`
- **Returns**: `{success, transferred}` or `{success, length, data}`

### start_read_poll
- **Inputs**: `path`, `endpoint`
- **Returns**: `{success, path, endpoint}`
- Use `get_read_data` to retrieve buffered reads

### get_read_data
- **Returns**: `{messages: [{path, data_hex, timestamp}, ...]}`
- Clears buffer after retrieval
