# SimpleCommKitAiWebSocketServerFastmcpp

Native C++ MCP (Model Context Protocol) server for WebSocket server management.
Exposes 8 MCP tools for AI agents to start, stop, send, broadcast, and manage
WebSocket server connections with zero Python dependency.

This is the C++ counterpart of `SimpleCommKitAiWebSocketServer`, providing lower latency
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
 │ ws_server_tools│ ← register_tools(): 8 MCP tools
 ├──────────────┤
 │ ws_server_state│ ← WsServerState singleton: server + client tracking + message buffer
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
# From the SimpleCommKitAiWebSocketServerFastmcpp directory
mkdir build && cd build
cmake ..
cmake --build .
```

Enable tests:
```bash
cmake .. -DSIMPLECOMMKITAIWEBSOCKETSERVERFASTMCPP_BUILD_TESTS=ON
cmake --build .
ctest
```

## Usage

```
simplecommkitaiwebsocketserver-fastmcpp [OPTIONS]

Options:
  --transport TRANSPORT   Transport protocol (default: stdio)
                          Choices: stdio, http, sse, streamable-http
  --host HOST             Host to bind to (default: 127.0.0.1)
  --port PORT             Port to bind to (default: 8011)
  --help, -h              Show this help message
```

### Examples

```bash
# stdio mode (default, for process-based MCP clients)
./simplecommkitaiwebsocketserver-fastmcpp

# SSE server on port 8011
./simplecommkitaiwebsocketserver-fastmcpp --transport sse --port 8011

# Streamable HTTP server
./simplecommkitaiwebsocketserver-fastmcpp --transport streamable-http

# HTTP server
./simplecommkitaiwebsocketserver-fastmcpp --transport http --port 8011
```

## MCP Tools

| # | Tool | Parameters | Description |
|---|------|-----------|-------------|
| 1 | `ws_server_start` | `port`, `host` | Start the WebSocket server |
| 2 | `ws_server_stop` | (none) | Stop the WebSocket server |
| 3 | `ws_server_status` | (none) | Check server state and client count |
| 4 | `ws_server_send` | `client_id`, `data`, `is_hex` | Send data to a specific client |
| 5 | `ws_server_broadcast` | `data`, `is_hex` | Broadcast data to all clients |
| 6 | `ws_server_get_messages` | (none) | Retrieve buffered received messages |
| 7 | `ws_server_set_max_connections` | `max_connections` | Set max concurrent connections |
| 8 | `ws_server_set_thread_num` | `thread_num` | Set worker thread count |

## Quick Start Flow

```
1. ws_server_start(port=8080, host="0.0.0.0")
2. ws_server_status()         ← check running and connected clients
3. ws_server_broadcast(data="Welcome!", is_hex=false)
4. ws_server_send(client_id=1, data="Hello client 1")
5. ws_server_get_messages()   ← retrieve client messages
6. ws_server_stop()
```

## Platform Support

- **Windows** ✓ (WinSock2 via libhv)
- **macOS** ✓ (BSD sockets via libhv)
- **Linux** ✓ (POSIX sockets via libhv)

## License

See the [LICENSE](../../LICENSE) file.
