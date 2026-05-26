# Troubleshooting

## Common Issues

### "No Bluetooth adapter available"
- Ensure Bluetooth is enabled in your OS settings
- On Linux, ensure `bluetoothd` is running: `systemctl status bluetooth`
- Run `get_adapters` to verify adapter detection

### "Device not found in scan results"
- Increase scan timeout: `scan_for(timeout_ms=10000)`
- Ensure the device is in range and powered on
- Some devices only advertise periodically

### "Failed to connect"
- Verify the device is connectable (not already connected to another host)
- Check that the address matches exactly (case-sensitive)
- On macOS, ensure the app has Bluetooth permission

### "Read/Write failed"
- Verify the characteristic UUID is correct
- Check that the characteristic supports the operation (use `services` to check capabilities)
- Some characteristics require pairing/authentication

### Notifications not received
- Ensure `notify` or `indicate` was called successfully
- The characteristic must support notifications (check with `services`)
- Call `get_notifications` to retrieve buffered data

## Platform-Specific Issues

### Windows
- Bluetooth must be enabled in Windows Settings
- Some BLE adapters require drivers
- WinRT may require the app to have a message pump on the main thread

### macOS
- App must be granted Bluetooth permission in System Preferences
- Addresses are UUIDs (different format from MAC addresses)
- CoreBluetooth may cache stale device information

### Linux
- Requires `bluez` and `bluetoothd`
- May need `sudo` for some operations
- `rfkill` may block Bluetooth: `rfkill unblock bluetooth`
