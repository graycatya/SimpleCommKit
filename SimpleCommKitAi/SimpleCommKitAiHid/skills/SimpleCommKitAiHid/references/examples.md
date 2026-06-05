# SimpleCommKitAiHid - Usage Examples

## Example: Complete HID Interaction Flow

```python
from simple_comm_kit_hid import SimpleCommKitHid, get_error_description

# Initialize
hid = SimpleCommKitHid()
hid.set_callback_error(lambda code: print(f"Error: {get_error_description(code)}"))

# Enumerate devices
devices = hid.get_available_devices()
print(f"Found {len(devices)} HID device(s)")
for d in devices:
    print(f"  {d.product_string} at {d.path} (VID={d.release_number:#06x})")

# Open first device
if devices:
    hid.init()
    if hid.open(devices[0].path):
        print(f"Opened: {devices[0].path}")

    # Set read callback
    hid.set_callback_on_read(lambda data: print(f"Read {len(data)} bytes: {data.hex()}"))

    # Write a report
    report = bytes([0x00, 0x01, 0x02, 0x03])
    written = hid.write(devices[0].path, report)
    print(f"Wrote {written} bytes")

    # Send feature report
    feature = bytes([0x05, 0x01])
    hid.send_feature_report(devices[0].path, feature)

    # Close
    hid.close(devices[0].path)

hid.exit()
```

## Example: Open by VID/PID

```python
from simple_comm_kit_hid import SimpleCommKitHid

hid = SimpleCommKitHid()
hid.init()

# Open a device with specific VID/PID (e.g., 0x1234:0x5678)
if hid.open(vendor_id=0x1234, product_id=0x5678):
    paths = hid.get_open_paths()
    print(f"Opened: {paths}")
    
    # Write a report
    hid.write(bytes([0x01, 0x02]))
    
    hid.close()

hid.exit()
```

## Example: Hotplug Monitoring

```python
from simple_comm_kit_hid import SimpleCommKitHid
import time

hid = SimpleCommKitHid()
hid.init()

# Set hotplug callback
def on_hotplug(added, removed):
    for d in added:
        print(f"Device connected: {d.product_string} at {d.path}")
    for d in removed:
        print(f"Device removed: {d.product_string} at {d.path}")

hid.set_callback_on_hotplug(on_hotplug)
hid.set_hotplug_poll_interval(1000)  # Check every 1 second
hid.start_hotplug(0, 0)  # Monitor all devices

print("Monitoring hotplug events. Press Ctrl+C to stop.")
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    pass

hid.stop_hotplug()
hid.exit()
```

## Example: Using the HTTP API

```bash
# Start server
simplecommkitaihid-http --port 8002

# In another terminal:

# List devices
curl "http://127.0.0.1:8002/devices"

# List devices filtered by VID/PID
curl "http://127.0.0.1:8002/devices?vendor_id=4660&product_id=22136"

# Open a device
curl -X POST "http://127.0.0.1:8002/open" \
  -H "Content-Type: application/json" \
  -d '{"vendor_id": 4660, "product_id": 22136, "readable": true}'

# Write data
curl -X POST "http://127.0.0.1:8002/device//dev/hidraw0/write" \
  -H "Content-Type: application/json" \
  -d '{"data": "00010203"}'

# Send feature report
curl -X POST "http://127.0.0.1:8002/device//dev/hidraw0/feature" \
  -H "Content-Type: application/json" \
  -d '{"data": "0501"}'

# Get open paths
curl "http://127.0.0.1:8002/open_paths"

# Stream read data (SSE)
curl -N "http://127.0.0.1:8002/device//dev/hidraw0/stream"

# Close device
curl -X POST "http://127.0.0.1:8002/close//dev/hidraw0"
```
