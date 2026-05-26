---
name: simple-comm-kit-ai-serialport
description: Use the SimpleCommKitAiSerialPort MCP server to enumerate, open, read, write, and monitor serial ports. This skill provides guidance on the recommended flow (list -> open -> read/write -> close) and handles platform-specific differences like COM ports on Windows vs /dev/tty* on Linux/macOS. Use when the user wants to interact with serial port hardware or debug serial connections.
---

# SimpleCommKitAiSerialPort

SimpleCommKitAiSerialPort is an AI-friendly Serial Port toolkit powered by simple_comm_kit_serialport. This skill provides instructions for using the SimpleCommKitAiSerialPort MCP server to interact with serial port devices directly from the host machine.

## Quick Start Flow

Always follow this sequence for serial port interactions:

1. **Discovery**: Call `get_available_ports` to list all serial ports on the system.
2. **Open**: Call `open_port` with your chosen port name and configuration.
3. **Information**: Call `port_info` to check the port status and current configuration.
4. **Interaction**: Use `read_port`/`read_all_port` to receive data, `write_port` to send hex data.
5. **Continuous Read**: Call `set_port_callback_on_read` then `get_buffered_data` periodically to collect incoming data without blocking.
6. **Cleanup**: Always call `close_port` when finished to release the port.

## Core Instructions

- **Discovery**: Always call `get_available_ports` first to see what ports are available.
- **Platform Differences**: Windows uses `COM3`, `COM4`, etc. Linux uses `/dev/ttyUSB0`, `/dev/ttyACM0`. macOS uses `/dev/tty.usbserial*`, `/dev/tty.usbmodem*`.
- **Baud Rate**: Common values are 9600, 115200, 921600. Custom values are supported (any positive integer).
- **Data Handling**: Binary data is returned as `data_hex` (always reliable) and `data_utf8` (convenience field). If the data is not valid UTF-8, invalid bytes are skipped, so `data_utf8` may be incomplete or empty. Use `data_hex` for protocol analysis and `data_utf8` for human-readable strings.
- **Continuous Reading**: Use `set_port_callback_on_read` to enable background buffering, then `get_buffered_data` to retrieve and clear the buffer.
- **Configuration**: Use `configure_port` to change baud rate, parity, stop bits etc. Only specify the parameters you want to change.
- **Flow Control**: 0=None (default), 1=Hardware (RTS/CTS), 2=Software (XON/XOFF).

## MCP Tools Reference

| Tool | Description | Parameters |
|------|-------------|------------|
| `get_available_ports` | List all available serial ports | None |
| `open_port` | Open a serial port with config | `port_name`, `baud_rate` (default: 9600), `parity` (0-4), `data_bits` (5-8), `stop_bits` (0-2), `flow_control` (0-2), `read_buffer_size` |
| `close_port` | Close a serial port | `port_name` |
| `port_info` | Get port status & config | `port_name` |
| `read_port` | Read up to N bytes | `port_name`, `size` (default: 1024) |
| `read_all_port` | Read all available bytes | `port_name` |
| `write_port` | Write hex data to port | `port_name`, `data` (hex string) |
| `flush_port` | Flush port buffers | `port_name`, `buffer` ("all"/"read"/"write") |
| `configure_port` | Update port configuration | `port_name` + any: `baud_rate`, `parity`, `data_bits`, `stop_bits`, `flow_control`, `read_buffer_size`, `read_interval_timeout`, `dtr`, `rts` |
| `set_port_callback_on_read` | Enable background data buffering | `port_name` |
| `get_buffered_data` | Get & clear buffered data | `port_name` |

## Additional Resources

- For detailed tool documentation and platform notes, see [the reference guide](references/REFERENCE.md).
- For concrete usage examples, see [examples.md](references/examples.md).
