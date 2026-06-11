# SimpleCommKitAiUsb Examples

## Python Example

```python
from SimpleCommKitPyUsb import SimpleCommKitUsb, UsbDeviceInfo

# Initialize
usb = SimpleCommKitUsb()
usb.set_Callback_Error(lambda code: print(f"Error: {code}"))
usb.init()

# Enumerate devices
devices = SimpleCommKitUsb.get_available_devices()
for d in devices:
    print(f"  {d.path}: {d.vendor_id:04X}:{d.product_id:04X} {d.product_string}")

# Open device and communicate
if devices:
    dev = devices[0]
    usb.open(dev.path)
    usb.claim_Interface(dev.path, 0)

    # Control transfer (get device descriptor)
    data = bytes(18)
    result = usb.control_transfer(dev.path, 0x80, 0x06, 0x0100, 0x0000, data)
    print(f"Descriptor: {result.hex()}")

    # Bulk transfer
    usb.bulk_transfer_out(dev.path, 0x01, bytes([0x01, 0x02, 0x03]))

    # Interrupt IN
    data = usb.interrupt_transfer_in(dev.path, 0x81, 64)
    print(f"Interrupt data: {data.hex() if data else 'none'}")

    # Continuous read with callback
    usb.set_callback_on_read(lambda info, data: print(f"[{info.path}] {data.hex()}"))
    usb.start_read_poll(dev.path, 0x81)

    # Hotplug monitoring
    usb.set_callback_on_hotplug(
        lambda added, removed: print(f"+{len(added)} -{len(removed)} devices")
    )
    usb.start_hotplug()

    import time
    time.sleep(5)

    # Cleanup
    usb.stop_hotplug()
    usb.stop_read_poll()
    usb.close()
    usb.exit()
```

## HTTP API (curl)

```bash
# List USB devices
curl http://localhost:8000/devices

# Open device by VID/PID
curl -X POST http://localhost:8000/open \
  -H "Content-Type: application/json" \
  -d '{"vid": "0x1234", "pid": "0x5678"}'

# Bulk OUT transfer
curl -X POST http://localhost:8000/device/1:3/bulk_out \
  -H "Content-Type: application/json" \
  -d '{"endpoint": "0x01", "data": "AB CD EF"}'

# Bulk IN transfer
curl -X POST "http://localhost:8000/device/1:3/bulk_in?endpoint=0x81&length=64"

# Control transfer
curl -X POST http://localhost:8000/device/1:3/control \
  -H "Content-Type: application/json" \
  -d '{"bm_request_type": "0x80", "b_request": "0x06", "w_value": "0x0100", "w_index": "0x0000", "length": 18}'

# Start read poll (data arrives via SSE)
curl "http://localhost:8000/read/stream"

# Close device
curl -X POST http://localhost:8000/close/1:3
```
