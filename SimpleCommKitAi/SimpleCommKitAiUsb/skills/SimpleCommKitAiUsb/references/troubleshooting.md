# SimpleCommKitAiUsb Troubleshooting

## Common Issues

### Device not found
- Ensure the device is connected and powered
- Check USB cable and port
- Try a different USB port
- On Linux, check `lsusb` output
- On Windows, check Device Manager

### Open failed
- Ensure no other program has the device open
- On Linux, check udev permissions (see below)
- On Windows, check that the correct driver (WinUSB/libusbK) is installed
- Try running as administrator/root

### Bulk/Interrupt transfer failed
- Ensure you claimed the correct interface
- Verify the endpoint address (IN = 0x8X, OUT = 0x0X)
- Check that the endpoint exists in the device descriptor
- Increase timeout value

### Read data not received
- Ensure `start_read_poll` was called with correct endpoint
- Use `get_read_data` to check for buffered data
- Check that the callback is working (use `set_callback_on_read` for debug)
- Verify the read poll interval is appropriate

### Hotplug not working
- Check if platform supports native hotplug (the module auto-detects)
- On systems without native support, polling is used — ensure interval is set
- Start hotplug BEFORE connecting/disconnecting devices

## Platform-Specific

### Linux udev Rules
```bash
# /etc/udev/rules.d/99-usb.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="1234", ATTR{idProduct}=="5678", MODE="0666"
```

### Windows Driver Setup
Use **Zadig** (https://zadig.akeo.ie/) to replace the default driver with WinUSB or libusbK.

### macOS
On macOS, libusb uses IOKit. Device access generally works without special configuration.
