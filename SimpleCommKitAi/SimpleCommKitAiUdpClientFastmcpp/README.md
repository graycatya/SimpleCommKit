# SimpleCommKitAiUdpClientFastmcpp

Native C++ MCP (Model Context Protocol) server for UDP client communication.
Exposes 5 MCP tools for AI agents to open, send, receive, and manage UDP sockets
with zero Python dependency.

This is the C++ counterpart of `SimpleCommKitAiUdpClient`, providing lower latency
through direct native library linkage.

## Architecture

```
MCP Client (AI Agent)
        │
stdio / sse / streamable-http / http
        │
 ┌──────┴──────┐
 │   main.cpp   │  ← Entry point, CLI parsing, server startup
 ├──────────────┤
 │  udp_tools   │  ← register_tools(): 5 MCP tools
 ├──────────────┤
 │  udp_state    │  ← UdpClientState singleton: client + message buffer
 ├──────────────┤
 │   fastmcpp   │  ← C++ MCP framework
 ├──────────────┤
 │SimpleCommKitUdp│ ← C++ UDP abstraction (libhv backend)
 └──────────────┘
```

## Build

Prerequisites:
- CMake 3.19+
- C++17 compiler
- The parent [SimpleCommKit](https://github.com/nicholasgriffintn/SimpleCommKit) project built

```bash
# From the SimpleCommKitAiUdpClientFastmcpp directory
mkdir build && cd build
cmake ..
cmake --build .
```

Enable tests:
```bash
cmake .. -DSIMPLECOMMKITAIUDPCLIENTFASTMCPP_BUILD_TESTS=ON
cmake --build .
ctest
```

## Usage

```
simplecommkitaiudpclient-fastmcpp [OPTIONS]

Options:
  --transport TRANSPORT   Transport protocol (default: stdio)
                          Choices: stdio, http, sse, streamable-http
  --host HOST             Host to bind to (default: 127.0.0.1)
  --port PORT             Port to bind to (default: 8008)
  --help, -h              Show this help message
```

### Examples

```bash
# stdio mode (default, for process-based MCP clients)
./simplecommkitaiudpclient-fastmcpp

# SSE server on port 8008
./simplecommkitaiudpclient-fastmcpp --transport sse --port 8008

# Streamable HTTP server
./simplecommkitaiudpclient-fastmcpp --transport streamable-http

# HTTP server
./simplecommkitaiudpclient-fastmcpp --transport http --port 8008
```

## MCP Tools

| # | Tool | Parameters | Description |
|---|------|-----------|-------------|
| 1 | `udp_open` | `local_port` (int, 0), `local_host` (str, "0.0.0.0") | Open a local UDP socket |
| 2 | `udp_close` | (none) | Close the UDP socket |
| 3 | `udp_status` | (none) | Check if socket is open |
| 4 | `udp_send_to` | `host` (str), `port` (int), `data` (str), `is_hex` (bool, false) | Send a datagram to the target |
| 5 | `udp_get_messages` | (none) | Retrieve buffered received datagrams |

## Quick Start Flow

```
1. udp_open()                                    ← open local UDP socket
2. udp_send_to(host="127.0.0.1", port=8002, data="Hello")
3. udp_get_messages()                            ← retrieve responses
4. udp_close()
```

## Platform Support

- **Windows** ✓ (WinSock2 via libhv)
- **macOS** ✓ (BSD sockets via libhv)
- **Linux** ✓ (POSIX sockets via libhv)

## License

See the [LICENSE](../../LICENSE) file.
