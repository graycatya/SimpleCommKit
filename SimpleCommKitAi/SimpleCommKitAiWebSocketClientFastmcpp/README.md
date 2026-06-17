# SimpleCommKitAiWebSocketClientFastmcpp

Native C++ MCP (Model Context Protocol) server for WebSocket client communication.
Exposes 8 MCP tools for AI agents to connect, send, receive, and manage WebSocket
connections with zero Python dependency.

This is the C++ counterpart of `SimpleCommKitAiWebSocketClient`, providing lower latency
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
 │  ws_tools    │  ← register_tools(): 8 MCP tools
 ├──────────────┤
 │  ws_state    │  ← WsClientState singleton: client + message buffer
 ├──────────────┤
 │   fastmcpp   │  ← C++ MCP framework
 ├──────────────┤
 │SimpleCommKitWebSocket│ ← C++ WebSocket abstraction (libhv backend)
 └──────────────┘
```

## Build

Prerequisites:
- CMake 3.19+
- C++17 compiler
- The parent [SimpleCommKit](https://github.com/nicholasgriffintn/SimpleCommKit) project built

```bash
# From the SimpleCommKitAiWebSocketClientFastmcpp directory
mkdir build && cd build
cmake ..
cmake --build .
```

Enable tests:
```bash
cmake .. -DSIMPLECOMMKITAIWEBSOCKETCLIENTFASTMCPP_BUILD_TESTS=ON
cmake --build .
ctest
```

## Usage

```
simplecommkitaiwebsocketclient-fastmcpp [OPTIONS]

Options:
  --transport TRANSPORT   Transport protocol (default: stdio)
                          Choices: stdio, http, sse, streamable-http
  --host HOST             Host to bind to (default: 127.0.0.1)
  --port PORT             Port to bind to (default: 8010)
  --help, -h              Show this help message
```

### Examples

```bash
# stdio mode (default, for process-based MCP clients)
./simplecommkitaiwebsocketclient-fastmcpp

# SSE server on port 8010
./simplecommkitaiwebsocketclient-fastmcpp --transport sse --port 8010

# Streamable HTTP server
./simplecommkitaiwebsocketclient-fastmcpp --transport streamable-http

# HTTP server
./simplecommkitaiwebsocketclient-fastmcpp --transport http --port 8010
```

## MCP Tools

| # | Tool | Parameters | Description |
|---|------|-----------|-------------|
| 1 | `ws_connect` | `url` (str) | Open a WebSocket connection to the given URL |
| 2 | `ws_disconnect` | (none) | Close the current WebSocket connection |
| 3 | `ws_status` | (none) | Check connection state |
| 4 | `ws_send` | `data` (str), `is_hex` (bool, false) | Send data to the server |
| 5 | `ws_get_messages` | (none) | Retrieve buffered received messages |
| 6 | `ws_set_reconnect` | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` | Configure auto-reconnect |
| 7 | `ws_set_connect_timeout` | `timeout_ms` (int, 5000) | Set connection timeout in ms |
| 8 | `ws_set_ping_interval` | `interval_ms` (int, 0) | Set ping interval (0=disable) |

## Quick Start Flow

```
1. ws_connect(url="ws://127.0.0.1:8080/ws")
2. ws_send(data="Hello", is_hex=false)
3. ws_get_messages()        ← retrieve responses
4. ws_disconnect()
```

## Platform Support

- **Windows** ✓ (WinSock2 via libhv)
- **macOS** ✓ (BSD sockets via libhv)
- **Linux** ✓ (POSIX sockets via libhv)

## License

See the [LICENSE](../../LICENSE) file.
