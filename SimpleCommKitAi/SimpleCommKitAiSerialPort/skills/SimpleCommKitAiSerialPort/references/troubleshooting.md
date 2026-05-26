# Troubleshooting - SimpleCommKitAiSerialPort

## Port not found

**Symptom**: `get_available_ports` returns an empty list or the expected port is missing.

**Possible causes**:
- The USB-to-serial adapter is not plugged in
- Driver not installed (common on Windows for CH340/CP2102 chips)
- Port is in use by another application (e.g. Arduino IDE, PuTTY)

**Check**:
- Windows: Open Device Manager → "Ports (COM & LPT)"
- Linux: `ls /dev/tty*` and check `dmesg | tail` after plugging in
- macOS: `ls /dev/tty.*` and check System Information → USB

## Port won't open

**Symptom**: `open_port` returns an error.

**Possible causes**:
- Another application has the port open
- Insufficient permissions (Linux: add user to `dialout` group)
- Incorrect port name

**Check**:
- Linux: `sudo usermod -a -G dialout $USER` then log out and back in
- macOS: Port may need to be released: `sudo kextunload` the driver

## Read returns empty data

**Symptom**: `read_port` returns `data_hex: ""` (empty).

**Possible causes**:
- The device isn't sending data
- Wrong baud rate (data arrives but is garbled → no valid bytes)
- Wrong port selected

**Check**:
- Try `read_all_port` to get any available bytes
- Verify baud rate matches the device
- Use `port_info` to check current configuration
- Try reading with a larger timeout or multiple times

## Write didn't work

**Symptom**: `write_port` succeeds but device doesn't respond.

**Possible causes**:
- Device expects different line ending (CR vs LF vs CRLF)
- Device needs specific DTR/RTS signaling
- Flow control mismatch

**Check**:
- Try `configure_port` to toggle DTR/RTS
- Common command endings: `\r` = `0D`, `\n` = `0A`, `\r\n` = `0D0A`
