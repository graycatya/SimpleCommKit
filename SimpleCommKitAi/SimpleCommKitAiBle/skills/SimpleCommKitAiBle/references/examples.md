# SimpleCommKitAiBle - Usage Examples

## Example: Complete BLE Interaction Flow

```python
from simple_comm_kit_ble import BleCentral, get_error_description

# Initialize
central = BleCentral()
central.set_callback_error(lambda code: print(f"Error: {get_error_description(code)}"))

# Get adapters
adapters = central.get_adapters()
print(f"Found {len(adapters)} adapter(s)")
for a in adapters:
    print(f"  {a.identifier} ({a.address})")

central.set_current_adapter(adapters[0])

# Scan
central.adapter_scan_for(5000)
results = central.adapter_get_scan_results()
print(f"Found {len(results)} peripheral(s)")
for p in results:
    print(f"  {p.address} RSSI={p.rssi}")

# Connect
target = results[0]
central.set_current_peripheral(target)
central.peripheral_connect()
print(f"Connected: {central.peripheral_is_connected()}")

# Discover services
services = central.peripheral_services()
for svc in services:
    print(f"Service: {svc.uuid}")
    for ch in svc.characteristics:
        caps = []
        if ch.can_read: caps.append("READ")
        if ch.can_write_request: caps.append("WRITE")
        if ch.can_notify: caps.append("NOTIFY")
        print(f"  {ch.uuid}: {', '.join(caps)}")

# Read a characteristic
if services and services[0].characteristics:
    ch = services[0].characteristics[0]
    if ch.can_read:
        data = central.peripheral_read(svc.uuid, ch.uuid)
        print(f"Read {len(data)} bytes: {data.hex()}")

# Subscribe to notifications
if services and services[0].characteristics:
    ch = services[0].characteristics[1] if len(services[0].characteristics) > 1 else services[0].characteristics[0]
    if ch.can_notify:
        notifications = []
        central.peripheral_notify(svc.uuid, ch.uuid, lambda data: notifications.append(data))
        import time
        time.sleep(5)
        print(f"Received {len(notifications)} notifications")
        central.peripheral_unsubscribe(svc.uuid, ch.uuid)

# Disconnect
central.peripheral_disconnect()
```

## Example: Descriptor Operations

```python
from simple_comm_kit_ble import BleCentral

central = BleCentral()
# ... (scan and connect as above)

# Read a descriptor (e.g., CCCD to check notification state)
desc_data = central.peripheral_read_descriptor(service_uuid, char_uuid, desc_uuid)
print(f"Descriptor data: {desc_data.hex()}")

# Write to a descriptor (e.g., enable notifications via CCCD)
central.peripheral_write_descriptor(service_uuid, char_uuid, desc_uuid, b"\x01\x00")

# Disconnect
central.peripheral_disconnect()
```

## Example: Using the HTTP API

```bash
# Start server
simplecommkitaible-http --port 8000

# In another terminal:
# Scan
curl -X POST "http://127.0.0.1:8000/scan?timeout_ms=5000"

# Connect
curl -X POST "http://127.0.0.1:8000/connect/AA:BB:CC:DD:EE:FF"

# Get services
curl "http://127.0.0.1:8000/device/AA:BB:CC:DD:EE:FF"

# Read
curl -X POST "http://127.0.0.1:8000/device/AA:BB:CC:DD:EE:FF/read/1800/2a00"

# Write
curl -X POST "http://127.0.0.1:8000/device/AA:BB:CC:DD:EE:FF/write/1800/2a00" \
  -H "Content-Type: application/json" \
  -d '{"data": "0100"}'

# Get notifications
curl "http://127.0.0.1:8000/device/AA:BB:CC:DD:EE:FF/notifications"

# Disconnect
curl -X POST "http://127.0.0.1:8000/disconnect/AA:BB:CC:DD:EE:FF"
```
