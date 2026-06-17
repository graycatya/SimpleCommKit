# Troubleshooting

## Common Issues

### "Failed to open COM3" / "Failed to open /dev/ttyUSB0"

- **Port in use**: Another application may have the port open. Close other terminal programs, serial monitors, or IDEs.
- **Permissions (Linux)**: Add your user to the `dialout` group: `sudo usermod -a -G dialout $USER` then log out and back in.
- **Permissions (Linux)**: Temporarily use `sudo` or set permissions: `sudo chmod 666 /dev/ttyUSB0`
- **Permissions (macOS)**: Usually no special permissions needed for `/dev/cu.*` devices.
- **Windows**: Check Device Manager to ensure the port exists and the driver is installed correctly.

### "Port not found: COM3"

- Verify the port name from `get_available_ports` results. Port names are case-sensitive on Linux/macOS.
- On Windows, use `COM3` (no backslashes).
- On Linux, verify the device file exists: `ls -la /dev/ttyUSB*`
- On macOS, verify: `ls -la /dev/cu.*`

### "Write failed" / No data received

- **Baud rate mismatch**: Both sides must use the same baud rate. Use `get_config` to verify.
- **Parity/data/stop mismatch**: Check that parity, data bits, and stop bits match the remote device.
- **Flow control**: If the remote device uses hardware flow control (RTS/CTS), enable it with `flow_control="hardware"`.
- **Wiring**: Verify TX/RX/GND connections. TX of one device goes to RX of the other.
- **Null modem**: For direct PC-to-PC connections, a null-modem cable or crossover adapter is needed.

### Read data not received (async/buffered)

- Data is buffered automatically via the On_Read callback after opening. Call `get_read_data` to retrieve it.
- If no data appears, try `read_all` for a synchronous read to verify the port is receiving data.
- Check that the read interval timeout is appropriate: `get_config` → `read_interval_timeout_ms`.

### Garbled / Corrupted data

- **Baud rate mismatch**: The most common cause. Verify both sides use the same baud rate.
- **Ground loop**: Ensure proper grounding between devices.
- **Cable length**: RS232 is typically limited to 15m at high speeds. Use shorter cables or lower baud rates.
- **Noise**: Shielded cables help in noisy environments.

### "Flush failed"

- Ensure the port is open before flushing.
- Some USB-serial adapters have limited flush support.

### DTR/RTS not working

- Not all serial devices support DTR/RTS control. USB-serial adapters typically do.
- Some devices use DTR for power; de-asserting may cause the device to reset.
- On Linux, check that your user has permission to control modem lines.

## Platform-Specific Issues

### Windows

- COM port numbers above COM9 may require the `\\.\COM10` prefix format. The `get_available_ports` output will show the correct format.
- Some USB-serial adapters (e.g., Prolific, FTDI) require driver installation.
- Windows 10/11 may assign a new COM port number when a USB-serial adapter is plugged into a different USB port.

### macOS

- Use `/dev/cu.*` devices (call-out), not `/dev/tty.*` (call-in). Call-out devices don't wait for DCD (Data Carrier Detect).
- Built-in Bluetooth serial ports appear as `/dev/cu.Bluetooth-Incoming-Port`.
- FTDI and CP210x drivers are built into macOS.

### Linux

- `/dev/ttyUSB*` — USB-serial adapters (FTDI, CP210x, CH340, PL2303)
- `/dev/ttyACM*` — USB modems and some Arduino boards
- `/dev/ttyS*` — Built-in hardware serial ports
- `dmesg | grep tty` shows kernel messages when a serial device is connected.
- The `setserial` command can show/set hardware serial port parameters.
- BR# (Baud Rate) settings above 115200 (e.g., 921600) may not be supported by all hardware.

### Getting Detailed Port Info

On each platform:
- **Windows**: Device Manager → Ports (COM & LPT) → Right-click → Properties
- **Linux**: `udevadm info --name=/dev/ttyUSB0 --attribute-walk`
- **macOS**: `system_profiler SPUSBDataType` for USB-serial adapters
