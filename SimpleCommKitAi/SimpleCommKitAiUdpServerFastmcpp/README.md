# SimpleCommKitAiUdpServerFastmcpp

Native C++ MCP (Model Context Protocol) server for UDP server management.
Exposes 6 MCP tools for AI agents to start, stop, send, broadcast, and receive
UDP datagrams with zero Python dependency.

This is the C++ counterpart of `SimpleCommKitAiUdpServer`, providing lower latency
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
 │udp_srv_tools │  ← register_tools(): 6 MCP tools
 ├──────────────┤
 │udp_srv_state │  ← UdpServerState singleton: server + message buffer
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
# From the SimpleCommKitAiUdpServerFastmcpp directory
mkdir build && cd build
cmake ..
cmake --build .
```

Enable tests:
```bash
cmake .. -DSIMPLECOMMKITAIUDPSERVERFASTMCPP_BUILD_TESTS=ON
cmake --build .
ctest
```

## Usage

```
simplecommkitaiupdserver-fastmcpp [OPTIONS]

Options:
  --transport TRANSPORT   Transport protocol (default: stdio)
                          Choices: stdio, http, sse, streamable-http
  --host HOST             Host to bind to (default: 127.0.0.1)
  --port PORT             Port to bind to (default: 8102)
  --help, -h              Show this help message
```

### Examples

```bash
# stdio mode (default, for process-based MCP clients)
./simplecommkitaiupdserver-fastmcpp

# SSE server on port 8102
./simplecommkitaiupdserver-fastmcpp --transport sse --port 8102

# Streamable HTTP server
./simplecommkitaiupdserver-fastmcpp --transport streamable-http

# HTTP server
./simplecommkitaiupdserver-fastmcpp --transport http --port 8102
```

## MCP Tools

| # | Tool | Parameters | Description |
|---|------|-----------|-------------|
| 1 | `udp_server_start` | `port` (int), `host` (str, "0.0.0.0") | Start listening for UDP datagrams |
| 2 | `udp_server_stop` | (none) | Stop the UDP server |
| 3 | `udp_server_status` | (none) | Check running state, host, port |
| 4 | `udp_server_send_to` | `host` (str), `port` (int), `data` (str), `is_hex` (bool) | Send datagram to a specific address |
| 5 | `udp_server_broadcast` | `data` (str), `is_hex` (bool) | Broadcast to 255.255.255.255 |
| 6 | `udp_server_get_messages` | (none) | Retrieve buffered datagrams with sender info |

## Quick Start Flow

```
1. udp_server_start(port=8002)                 ← start listening
2. udp_server_status()                         ← verify running
3. udp_server_get_messages()                   ← check for incoming datagrams
4. udp_server_send_to(host="127.0.0.1", port=8003, data="Reply")
5. udp_server_broadcast(data="Announcement")   ← send to all on subnet
6. udp_server_stop()
```

## Platform Support

- **Windows** ✓ (WinSock2 via libhv)
- **macOS** ✓ (BSD sockets via libhv)
- **Linux** ✓ (POSIX sockets via libhv)

## License

See the [LICENSE](../../LICENSE) file.
