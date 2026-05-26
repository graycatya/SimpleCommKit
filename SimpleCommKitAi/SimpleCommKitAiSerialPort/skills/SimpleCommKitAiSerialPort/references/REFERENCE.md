# SimpleCommKitAiSerialPort Reference Guide

## MCP Tool Details

### get_available_ports

List all available serial ports on the host system.

**Returns**: List of `{"port_name": str, "description": str, "hardware_id": str}` objects.

**Platform Notes**:
- Windows: Port names like `COM1`, `COM3` etc.
- Linux: Port names like `/dev/ttyUSB0`, `/dev/ttyACM0`, `/dev/ttyS0`
- macOS: Port names like `/dev/tty.usbserial-*`, `/dev/tty.usbmodem*`

---

### open_port

Open a serial port with configuration parameters.

**Parameters**:
- `port_name` (str): Port name from `get_available_ports`
- `baud_rate` (int, default: 9600): Baud rate. Any positive integer is valid (e.g. 9600, 115200, 921600).
- `parity` (int, default: 0): 0=None, 1=Odd, 2=Even, 3=Mark, 4=Space
- `data_bits` (int, default: 8): 5, 6, 7, or 8
- `stop_bits` (int, default: 0): 0=One, 1=OneAndHalf, 2=Two
- `flow_control` (int, default: 0): 0=None, 1=Hardware (RTS/CTS), 2=Software (XON/XOFF)
- `read_buffer_size` (int, default: 4096): Internal read buffer size

**Note**: After opening, the `on_read` callback is automatically set up to buffer incoming data. Retrieve buffered data with `get_buffered_data`.

---

### close_port

Close a serial port.

**Parameters**:
- `port_name` (str): Port name to close

**Note**: Always close ports when you're done to free the resource for other applications.

---

### port_info

Get current status and configuration of an open port.

**Returns**: Object with `port_name`, `is_open`, `baud_rate`, `parity`, `data_bits`, `stop_bits`, `flow_control`, `read_buffer_size`, `read_interval_timeout`, `last_error`, `last_error_msg`.

---

### read_port

Read up to `size` bytes from the serial port. This is a blocking read.

**Parameters**:
- `port_name` (str): Port name
- `size` (int, default: 1024): Maximum bytes to read

**Returns**: `{"port_name": str, "size": int, "data_hex": str, "data_utf8": str}`

---

### read_all_port

Read all currently available bytes from the serial port.

**Parameters**:
- `port_name` (str): Port name

---

### write_port

Write hex-encoded data to the serial port.

**Parameters**:
- `port_name` (str): Port name
- `data` (str): Hex string (e.g. `"48656C6C6F"` for "Hello")

**Returns**: `{"port_name": str, "bytes_written": int}`

**Example**: Write "Hello" → `data = "48656c6c6f"`

---

### flush_port

Flush serial port buffers.

**Parameters**:
- `port_name` (str): Port name
- `buffer` (str, default: "all"): Which buffer to flush - "all", "read", or "write"

---

### configure_port

Update serial port configuration. Only provide the parameters you want to change; others remain unchanged.

**Parameters** (all optional):
- `port_name` (str): Port name
- `baud_rate` (int): New baud rate
- `parity` (int): 0-4
- `data_bits` (int): 5-8
- `stop_bits` (int): 0-2
- `flow_control` (int): 0-2
- `read_buffer_size` (int): Buffer size in bytes
- `read_interval_timeout` (int): Timeout in milliseconds
- `dtr` (bool): Set or clear DTR line
- `rts` (bool): Set or clear RTS line

**Note**: Changing baud rate, parity, data bits, or stop bits while the port is open may have platform-specific behavior. It's recommended to close the port, reconfigure, then reopen.

---

### set_port_callback_on_read

Start buffering incoming data in the background.

**Parameters**:
- `port_name` (str): Port name

After calling this, all incoming data is buffered and can be retrieved with `get_buffered_data`.

---

### get_buffered_data

Retrieve and clear all buffered incoming data.

**Parameters**:
- `port_name` (str): Port name

**Returns**: List of `{"data_hex": str, "data_utf8": str}` objects.

Also clears the buffer so future calls will only return new data.

## Error Codes

Use `get_error_description(error_code)` to convert error codes to human-readable strings.

Common errors:
- `port not open`: The port needs to be opened before reading/writing
- `init error`: Port initialization failed (bad parameters or port not available)
- `open error`: Port could not be opened (in use by another application?)
- `read/write error`: I/O operation failed
- `flush error`: Buffer flush failed
