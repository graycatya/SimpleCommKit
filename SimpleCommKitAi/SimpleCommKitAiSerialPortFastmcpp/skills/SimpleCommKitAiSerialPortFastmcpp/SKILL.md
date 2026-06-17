---
name: simple-comm-kit-ai-serialport-fastmcpp
description: Use the SimpleCommKitAiSerialPortFastmcpp MCP server to enumerate, open, and interact with serial (COM) ports via native C++. This skill provides guidance on the recommended flow (enumerate -> open -> write/read) and handles platform-specific differences like port names on different OSes. Use when the user wants to interact with serial port hardware or debug RS232/RS485 connections.
---

# SimpleCommKitAiSerialPortFastmcpp (C++)

SimpleCommKitAiSerialPortFastmcpp is a native C++ AI-friendly serial-port toolkit powered by the SimpleCommKitSerialPort C++ library and the fastmcpp MCP framework. This skill provides instructions for using the SimpleCommKitAiSerialPortFastmcpp MCP server to interact with serial (COM) ports directly from the host machine.

## Quick Start Flow

Always follow this sequence for serial port interactions:

1. **Enumerate**: Call `get_available_ports` to list available serial ports on the host.
2. **Open**: Call `open` with the port name (e.g. `"COM3"`, `"/dev/ttyUSB0"`) and desired baud rate/configuration.
3. **Interaction**: Use `write` to send hex data, `read`/`read_all` for synchronous reads, or `get_read_data` to retrieve asynchronously buffered data.
4. **Cleanup**: Always call `close` when finished to release the port.

## Core Instructions

- **Port Names**: Serial port names are platform-specific. On Windows they look like `COM1`, `COM3`, etc. On Linux they look like `/dev/ttyUSB0`, `/dev/ttyS0`, `/dev/ttyACM0`. On macOS they look like `/dev/cu.usbserial-*` or `/dev/tty.usbserial-*`. Always use the `port_name` field from `get_available_ports` results.
- **Configuration**: Serial ports require matching configuration on both ends. The default settings are 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control. Use `get_config` to check current settings and `set_config` to change them.
- **Data Handling**: Binary data is sent and received as hex strings. Input data is returned as `data_hex` (always reliable) and `data_utf8` (convenience field). If the data is not valid UTF-8, invalid bytes are skipped, so `data_utf8` may be incomplete or empty.
- **Async Read Buffering**: When a port is opened, a read callback is automatically installed. Incoming data is buffered asynchronously. Use `get_read_data` to retrieve and clear the buffer.
- **Synchronous Read**: Use `read` or `read_all` for direct synchronous reads. These return data immediately from the port's input buffer.
- **Flow Control**: Use `none` (default), `hardware` (RTS/CTS), or `software` (XON/XOFF) flow control. Use `set_dtr`/`set_rts` for manual signal control.

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `get_available_ports` | List available serial ports | None |
| `open` | Open a serial port | `port_name`, `baud_rate`, `parity`, `data_bits`, `stop_bits`, `flow_control`, `read_buffer_size`, `read_interval_timeout_ms` |
| `close` | Close a serial port | `port_name` (optional, closes all if omitted) |
| `write` | Write data to a port | `port_name`, `data` (hex string) |
| `read` | Read up to N bytes | `port_name`, `size` |
| `read_all` | Read all available data | `port_name` |
| `get_config` | Get port configuration | `port_name` |
| `set_config` | Update port configuration | `port_name`, `baud_rate`, `parity`, `data_bits`, `stop_bits`, `flow_control`, `read_buffer_size`, `read_interval_timeout_ms` |
| `flush_buffers` | Flush read/write buffers | `port_name`, `direction` |
| `set_dtr` | Control DTR signal line | `port_name`, `set` |
| `set_rts` | Control RTS signal line | `port_name`, `set` |
| `get_open_ports` | List currently open ports | None |
| `get_read_data` | Retrieve buffered async data | `port_name` |
| `get_error` | Get and clear last error | `port_name` |

## Additional Resources

- For detailed tool documentation and platform notes, see [the reference guide](references/REFERENCE.md).
- For concrete usage examples, see [examples.md](references/examples.md).
- For troubleshooting common issues, see [troubleshooting.md](references/troubleshooting.md).
