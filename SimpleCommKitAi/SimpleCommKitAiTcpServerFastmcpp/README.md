# SimpleCommKitAiTcpServerFastmcpp

Native C++ MCP (Model Context Protocol) server for TCP server management.
Exposes 6 MCP tools for AI agents to start, stop, send, broadcast, and monitor
a TCP server with zero Python dependency.

This is the C++ counterpart of `SimpleCommKitAiTcpServer`, providing lower latency
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
 │tcp_server_to│  ← register_tools(): 6 MCP tools
 │    ols       │
 ├──────────────┤
 │tcp_server_st│  ← TcpServerState singleton: server + message buffer
 │     ate      │
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
# From the SimpleCommKitAiTcpServerFastmcpp directory
mkdir build && cd build
cmake ..
cmake --build .
```

Enable tests:
```bash
cmake .. -DSIMPLECOMMKITAITCPSERVERFASTMCPP_BUILD_TESTS=ON
cmake --build .
ctest
```

## Usage

```
simplecommkitaitcpserver-fastmcpp [OPTIONS]

Options:
  --transport TRANSPORT   Transport protocol (default: stdio)
                          Choices: stdio, http, sse, streamable-http
  --host HOST             Host to bind to (default: 127.0.0.1)
  --port PORT             Port to bind to (default: 8007)
  --help, -h              Show this help message
```

### Examples

```bash
# stdio mode (default, for process-based MCP clients)
./simplecommkitaitcpserver-fastmcpp

# SSE server on port 8007
./simplecommkitaitcpserver-fastmcpp --transport sse --port 8007

# Streamable HTTP server
./simplecommkitaitcpserver-fastmcpp --transport streamable-http

# HTTP server
./simplecommkitaitcpserver-fastmcpp --transport http --port 8007
```

## MCP Tools

| # | Tool | Parameters | Description |
|---|------|-----------|-------------|
| 1 | `tcp_server_start` | `port` (int), `host` (str, "0.0.0.0") | Start listening on port |
| 2 | `tcp_server_stop` | (none) | Stop server and disconnect all clients |
| 3 | `tcp_server_status` | (none) | Check running state and connection count |
| 4 | `tcp_server_send` | `client_id` (int), `data` (str), `is_hex` (bool) | Send data to a specific client |
| 5 | `tcp_server_broadcast` | `data` (str), `is_hex` (bool) | Broadcast data to all clients |
| 6 | `tcp_server_get_messages` | (none) | Retrieve and clear buffered client messages |

## Quick Start Flow

```
1. tcp_server_start(port=8080)
2. tcp_server_status()          ← check server is running
3. tcp_server_get_messages()    ← retrieve client messages
4. tcp_server_send(client_id=1, data="ACK")
5. tcp_server_broadcast(data="Shutdown in 5m")
6. tcp_server_stop()
```

## Platform Support

- **Windows** ✓ (WinSock2 via libhv)
- **macOS** ✓ (BSD sockets via libhv)
- **Linux** ✓ (POSIX sockets via libhv)

## License

See the [LICENSE](../../LICENSE) file.
