# Troubleshooting

## Common Issues

### "No HID device found"
- Ensure the HID device is connected to your computer.
- On Linux, check if the device appears: `ls /dev/hidraw*`
- Try without VID/PID filters: `get_available_devices(0, 0)`

### "Failed to open device"
- On Linux, you may need root permissions or udev rules.
  Add a udev rule: `SUBSYSTEM=="hidraw", MODE="0666"` in `/etc/udev/rules.d/99-hidraw.rules`
  Then run: `sudo udevadm control --reload-rules && sudo udevadm trigger`
- On Linux, the device may be in use by another process. Check: `sudo fuser /dev/hidraw0`
- Verify the device path is correct from `get_available_devices`

### "Write failed" / "Feature report failed"
- Ensure the device is open: use `get_open_paths` to verify.
- The report length must match the device's report descriptor. Check the HID report descriptor for expected sizes.
- Some devices require a specific report ID as the first byte.
- Feature reports require the device to use the HID feature report interface.

### Read data not received
- Ensure device was opened with `readable=True` (the default).
- Check read configuration: use `get_device_list` or the config endpoint.
- Increase `read_poll_interval_ms` (lower value = more frequent polling).
- Use `set_read_config` to adjust `data_length` if reports are larger than 64 bytes.

### Hotplug not detecting devices
- Hotplug uses polling; make sure `poll_interval_ms` is reasonable (e.g., 1000).
- Hotplug detects changes in device list, not individual report data.
- Hotplug only works for USB HID devices on most platforms.

## Platform-Specific Issues

### Windows
- HID devices are automatically detected; no driver installation needed for standard HID.
- Device paths use Windows device path format: `\\?\HID#...`

### macOS
- HID access is generally unrestricted on macOS.
- Device paths are IOKit service paths.

### Linux
- Requires `hidraw` kernel module: `modprobe hidraw`
- Permissions are the most common issue; use `sudo` or udev rules.
- Some devices may be claimed by the `usbhid` driver; check `dmesg` for errors.
- For Bluetooth HID devices, ensure Bluetooth is working.
