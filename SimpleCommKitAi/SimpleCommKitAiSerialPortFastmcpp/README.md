# SimpleCommKitAiSerialPortFastmcpp

Native C++ MCP server for serial (COM) port communication, built on [fastmcpp](https://github.com/nicholasgriffintn/fastmcpp) and [SimpleCommKitSerialPort](../src/SimpleCommKitSerialPort).

## Overview

`SimpleCommKitAiSerialPortFastmcpp` exposes 14 MCP tools for AI agents to enumerate, open, read, write, configure, and monitor serial ports — with native C++ performance and zero Python dependency.

## Architecture

```
AI Agent (MCP Client)
       │
       ▼ stdio / sse / streamable-http
┌──────────────────────────────────────┐
│  SimpleCommKitAiSerialPortFastmcpp   │  (C++ MCP Server)
├──────────────────────────────────────┤
│  fastmcpp (C++ MCP framework)        │
├──────────────────────────────────────┤
│  SimpleCommKitSerialPort (C++ lib)   │
├──────────────────────────────────────┤
│  libcserialport (C serial backend)   │
└──────────────────────────────────────┘
```

## Building

### Prerequisites

- CMake 3.19+
- C++17 compiler
- The parent project (`SimpleCommKit`) must be configured first, which builds `SimpleCommKitSerialPort` and `fastmcpp`

### Build

```bash
cd SimpleCommKit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target simplecommkitaiserialport-fastmcpp
```

### Build with Tests

```bash
cmake .. -DSIMPLECOMMKITAISERIALPORTFASTMCPP_BUILD_TESTS=ON
cmake --build . --target simplecommkitaiserialport-fastmcpp serial_utility_test
ctest
```

## Usage

### MCP Client Configuration

Add to your MCP client (Claude Desktop, Cursor, etc.):

```json
{
  "mcpServers": {
    "simplecommkitaiserialport-fastmcpp": {
      "command": "simplecommkitaiserialport-fastmcpp"
    }
  }
}
```

### Command Line

```bash
# stdio mode (default) — used by MCP clients
simplecommkitaiserialport-fastmcpp

# SSE mode — recommended for remote access
simplecommkitaiserialport-fastmcpp --transport sse --host 127.0.0.1 --port 8005

# Streamable HTTP mode
simplecommkitaiserialport-fastmcpp --transport streamable-http --port 8005

# Simple HTTP mode
simplecommkitaiserialport-fastmcpp --transport http --port 8005
```

## Available Tools

| Tool | Description |
|------|-------------|
| `get_available_ports` | List all available serial ports on the host |
| `open` | Open a serial port with configuration (baud, parity, etc.) |
| `close` | Close a specific port or all open ports |
| `write` | Write hex data to a serial port |
| `read` | Read up to N bytes synchronously from a port |
| `read_all` | Read all available data from a port's input buffer |
| `get_config` | Get current port configuration |
| `set_config` | Update port configuration |
| `flush_buffers` | Flush read/write/both buffers |
| `set_dtr` | Control DTR signal line |
| `set_rts` | Control RTS signal line |
| `get_open_ports` | List currently open ports |
| `get_read_data` | Retrieve and clear buffered read data (async callback) |
| `get_error` | Get and clear the last error from a port |

## Quick Start Flow

1. `get_available_ports` → find your serial port
2. `open` → open with desired baud rate and settings
3. `write` → send hex data
4. `read` / `get_read_data` → receive data
5. `close` → clean up

## Platform Support

- **Windows**: COM1, COM2, etc.
- **macOS**: /dev/cu.*, /dev/tty.*
- **Linux**: /dev/ttyUSB*, /dev/ttyS*, /dev/ttyACM*

## License

Apache 2.0 — See [LICENSE](../../LICENSE)
