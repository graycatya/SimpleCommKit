# Troubleshooting

## Common Issues

### "Bluetooth is disabled" (bluetooth_enabled returns false)

- **macOS**: Check System Preferences → Bluetooth. Ensure Bluetooth is turned on.
- **Linux**: Run `sudo systemctl start bluetooth` and `sudo rfkill unblock bluetooth`.
- **Windows**: Open Settings → Bluetooth & devices → Turn on Bluetooth.

### "Peripheral not found" (scan_for returns empty or connect fails)

1. Ensure the BLE device is powered on and in range (typically < 10m).
2. Some devices only advertise when in pairing mode. Check the device manual.
3. Increase scan timeout: `scan_for(timeout_ms=10000)`.
4. Make sure no other application is connected to the device.

### "BLE hardware not available"

This warning means the `SimpleCommKitBle` C++ library failed to initialize. Common causes:
- No Bluetooth adapter detected on the system
- Missing platform BLE libraries (e.g., SimpleBLE dependencies)
- Permission issues on Linux (try running with sudo)

### Connection drops unexpectedly

- BLE range is typically 10 meters. Move closer to the device.
- Some devices have aggressive power saving and disconnect after idle periods.
- Ensure the device battery is not depleted.
- Check for radio interference (Wi-Fi, USB 3.0, microwaves).

### Write fails with error

- Ensure you are using `write_request` for characteristics that require acknowledgment.
- Use `write_command` only for characteristics that support write-without-response.
- Check the characteristic's capabilities with `services` to verify `can_write_request` or `can_write_command`.

### Notifications not received

1. Verify the characteristic supports notifications (`can_notify` in `services` output).
2. After calling `notify`, wait some time before calling `get_notifications`.
3. Some devices require the CCCD descriptor (`2902`) to be written first. Try:
   ```
   descriptor_write(address="...", service_uuid="...", characteristic_uuid="...",
                    descriptor_uuid="2902", data="0100")
   ```
   Then call `notify`.

### macOS: "The operation couldn't be completed" errors

- macOS requires Bluetooth permission. Check System Preferences → Privacy → Bluetooth.
- The application binary must be signed or run from Terminal.
- CoreBluetooth may cache old connections. Try toggling Bluetooth off/on.

### Linux: "Permission denied" or "Operation not permitted"

```bash
# Add user to bluetooth group
sudo usermod -a -G bluetooth $USER

# Or set capabilities on the binary
sudo setcap 'cap_net_raw,cap_net_admin+eip' simplecommkitaible-fastmcpp
```

## Getting Debug Output

The server writes diagnostic information to stderr. Run from terminal to see debug output:

```bash
simplecommkitaible-fastmcpp 2>debug.log
```

Or run in SSE/HTTP mode and check the terminal output for error messages.
