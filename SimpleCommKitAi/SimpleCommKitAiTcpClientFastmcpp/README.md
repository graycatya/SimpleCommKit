# SimpleCommKitAiTcpClientFastmcpp

Native C++ MCP (Model Context Protocol) server for TCP client communication.
Exposes 6 MCP tools for AI agents to connect, send, receive, and manage TCP connections
with zero Python dependency.

This is the C++ counterpart of `SimpleCommKitAiTcpClient`, providing lower latency
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
 │  tcp_tools   │  ← register_tools(): 6 MCP tools
 ├──────────────┤
 │  tcp_state    │  ← TcpClientState singleton: client + message buffer
 ├──────────────┤
 │   fastmcpp   │  ← C++ MCP framework
 ├──────────────┤
 │SimpleCommKitTcp│ ← C++ TCP abstraction (libhv backend)
 └──────────────┘
```

## Build

Prerequisites:
- CMake 3.19+
- C++17 compiler
- The parent [SimpleCommKit](https://github.com/nicholasgriffintn/SimpleCommKit) project built

```bash
# From the SimpleCommKitAiTcpClientFastmcpp directory
mkdir build && cd build
cmake ..
cmake --build .
```

Enable tests:
```bash
cmake .. -DSIMPLECOMMKITAITCPCLIENTFASTMCPP_BUILD_TESTS=ON
cmake --build .
ctest
```

## Usage

```
simplecommkitaitcpclient-fastmcpp [OPTIONS]

Options:
  --transport TRANSPORT   Transport protocol (default: stdio)
                          Choices: stdio, http, sse, streamable-http
  --host HOST             Host to bind to (default: 127.0.0.1)
  --port PORT             Port to bind to (default: 8006)
  --help, -h              Show this help message
```

### Examples

```bash
# stdio mode (default, for process-based MCP clients)
./simplecommkitaitcpclient-fastmcpp

# SSE server on port 8006
./simplecommkitaitcpclient-fastmcpp --transport sse --port 8006

# Streamable HTTP server
./simplecommkitaitcpclient-fastmcpp --transport streamable-http

# HTTP server
./simplecommkitaitcpclient-fastmcpp --transport http --port 8006
```

## MCP Tools

| # | Tool | Parameters | Description |
|---|------|-----------|-------------|
| 1 | `tcp_connect` | `host` (str), `port` (int, 8080) | Connect to a TCP server |
| 2 | `tcp_disconnect` | (none) | Disconnect from the server |
| 3 | `tcp_status` | (none) | Check connection state |
| 4 | `tcp_send` | `data` (str), `is_hex` (bool, false) | Send data to the server |
| 5 | `tcp_get_messages` | (none) | Retrieve buffered received messages |
| 6 | `tcp_set_reconnect` | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` | Configure auto-reconnect |

## Quick Start Flow

```
1. tcp_connect(host="127.0.0.1", port=8080)
2. tcp_send(data="Hello", is_hex=false)
3. tcp_get_messages()        ← retrieve responses
4. tcp_disconnect()
```

## Platform Support

- **Windows** ✓ (WinSock2 via libhv)
- **macOS** ✓ (BSD sockets via libhv)
- **Linux** ✓ (POSIX sockets via libhv)

## License

See the [LICENSE](../../LICENSE) file.
