# SimpleCommKitAiSerialPortFastmcpp - Reference Guide

## Architecture

```
AI Agent (Cursor / Claude Code / Windsurf)
       │
       ▼ MCP Protocol (stdio / sse / streamable-http)
┌──────────────────────────────────────┐
│  SimpleCommKitAiSerialPortFastmcpp   │  ← This package (C++ native)
│  (MCP Server)                        │
├──────────────────────────────────────┤
│  fastmcpp                            │  ← C++ MCP framework
├──────────────────────────────────────┤
│  SimpleCommKitSerialPort             │  ← C++ serial port abstraction
├──────────────────────────────────────┤
│  libcserialport                      │  ← Cross-platform serial
│  (Win32 / termios / IOKit)           │
└──────────────────────────────────────┘
```

Unlike the Python variant (`SimpleCommKitAiSerialPort`), this implementation is a **native C++ binary** that links directly to `SimpleCommKitSerialPort` and `fastmcpp`, offering lower latency and zero Python dependency.

## MCP Server Usage

The MCP server can be configured in any MCP-compatible AI client:

```json
{
  "mcpServers": {
    "simplecommkitaiserialport-fastmcpp": {
      "command": "simplecommkitaiserialport-fastmcpp"
    }
  }
}
```

### Transport Modes

- **stdio** (default): Standard input/output, used by most MCP clients
- **sse**: Server-Sent Events transport (recommended for remote access)
- **streamable-http**: Streamable HTTP per MCP spec 2025-03-26
- **http**: Simple HTTP POST transport

```bash
# stdio mode (default)
simplecommkitaiserialport-fastmcpp

# SSE mode
simplecommkitaiserialport-fastmcpp --transport sse --host 127.0.0.1 --port 8005

# Streamable HTTP mode
simplecommkitaiserialport-fastmcpp --transport streamable-http --host 0.0.0.0 --port 8005

# HTTP mode
simplecommkitaiserialport-fastmcpp --transport http --port 8005
```

## MCP Tools Detail

### get_available_ports

List all available serial ports on the host system.

**Parameters:** None

**Returns:** Array of port info objects, each with `port_name`, `description`, `hardware_id`.

### open

Open a serial port for communication.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Port name (e.g. "COM3", "/dev/ttyUSB0") |
| baud_rate | integer | 115200 | Baud rate (e.g. 9600, 115200, 921600) |
| parity | string | "none" | Parity: none, odd, even, mark, space |
| data_bits | integer | 8 | Data bits: 5, 6, 7, 8 |
| stop_bits | string | "one" | Stop bits: one, one_and_half, two |
| flow_control | string | "none" | Flow control: none, hardware, software |
| read_buffer_size | integer | 4096 | Internal buffer size in bytes |
| read_interval_timeout_ms | integer | 100 | Read timeout in milliseconds |

**Returns:** `{"message": "...", "port_name": "...", "baud_rate": ..., "parity": "...", ...}`

### close

Close a serial port (or all ports).

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | "" | Port name (omit to close all) |

**Returns:** `{"message": "..."}`

### write

Write hex data to an open serial port.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |
| data | string | (required) | Hex string (e.g. "48656C6C6F" or "48 65 6C 6C 6F") |

**Returns:** `{"message": "...", "port_name": "...", "bytes_written": N}`

### read

Synchronously read up to `size` bytes from a port.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |
| size | integer | 1024 | Maximum bytes to read |

**Returns:** `{"port_name": "...", "data_hex": "...", "data_utf8": "...", "data_length": N}`

### read_all

Synchronously read all currently available data from a port's input buffer.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |

**Returns:** `{"port_name": "...", "data_hex": "...", "data_utf8": "...", "data_length": N}`

### get_config

Get the current configuration of a serial port.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |

**Returns:** `{"port_name": "...", "baud_rate": ..., "parity": "...", "data_bits": ..., "stop_bits": "...", "flow_control": "...", "read_buffer_size": ..., "read_interval_timeout_ms": ..., "is_open": bool}`

### set_config

Update configuration of an open serial port. Only non-zero/non-empty values will be changed.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |
| baud_rate | integer | 0 | New baud rate (0 = no change) |
| parity | string | "" | New parity (empty = no change) |
| data_bits | integer | 0 | New data bits (0 = no change) |
| stop_bits | string | "" | New stop bits (empty = no change) |
| flow_control | string | "" | New flow control (empty = no change) |
| read_buffer_size | integer | 0 | New buffer size (0 = no change) |
| read_interval_timeout_ms | integer | -1 | New timeout (-1 = no change) |

**Returns:** Updated config JSON (same as `get_config`).

### flush_buffers

Flush read and/or write buffers.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |
| direction | string | "both" | Which buffer: read, write, or both |

**Returns:** `{"message": "...", "port_name": "..."}`

### set_dtr

Control DTR (Data Terminal Ready) signal line.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |
| set | boolean | true | True to assert, false to de-assert |

**Returns:** `{"message": "...", "port_name": "...", "dtr": bool}`

### set_rts

Control RTS (Request To Send) signal line.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |
| set | boolean | true | True to assert, false to de-assert |

**Returns:** `{"message": "...", "port_name": "...", "rts": bool}`

### get_open_ports

List currently open/managed serial ports.

**Parameters:** None

**Returns:** `{"open_ports": [{"port_name": "...", "is_open": bool}, ...], "open_count": N}`

### get_read_data

Retrieve and clear asynchronously buffered read data. Data is buffered automatically after opening a port.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |

**Returns:** Array of `{"port_name": "...", "data_hex": "...", "data_utf8": "...", "data_length": N}`

### get_error

Get and clear the last error from a serial port.

**Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| port_name | string | (required) | Name of the open port |

**Returns:** `{"port_name": "...", "error_code": ..., "error_message": "...", "error_description": "..."}`

## Platform Notes

### Linux
- Port names: `/dev/ttyUSB0`, `/dev/ttyS0`, `/dev/ttyACM0`, etc.
- May require permissions: add user to `dialout` group or use `sudo`
- `udev` rules can create persistent symlinks for specific devices
- Check available ports: `ls /dev/tty*`

### macOS
- Port names: `/dev/cu.usbserial-*`, `/dev/tty.usbserial-*`, `/dev/cu.Bluetooth-Incoming-Port`
- Use `/dev/cu.*` (call-out) devices; they don't wait for DCD
- No special permissions typically needed

### Windows
- Port names: `COM1`, `COM2`, `COM3`, etc.
- Virtual COM ports from USB-serial adapters are automatically assigned
- Check available ports in Device Manager under "Ports (COM & LPT)"
- No special permissions needed
