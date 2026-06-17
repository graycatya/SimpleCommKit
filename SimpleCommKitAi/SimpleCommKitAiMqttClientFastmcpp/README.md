# SimpleCommKitAiMqttClientFastmcpp

Native C++ MCP server for MQTT client communication, built on [fastmcpp](https://github.com/nicholasgriffintn/fastmcpp) and [SimpleCommKitMqttClient](../src/SimpleCommKitMqttClient).

## Overview

`SimpleCommKitAiMqttClientFastmcpp` is the C++ counterpart of `SimpleCommKitAiMqttClient`. It exposes **10 MCP tools** for AI agents to connect to MQTT brokers, publish and subscribe to topics, retrieve buffered messages, and configure connection parameters — with native C++ performance and zero Python dependency.

## Architecture

```
AI Agent (MCP Client)
       │
       ▼ stdio / sse / streamable-http
┌──────────────────────────────────────┐
│  SimpleCommKitAiMqttClientFastmcpp   │  (C++ MCP Server)
├──────────────────────────────────────┤
│  fastmcpp (C++ MCP framework)        │
├──────────────────────────────────────┤
│  SimpleCommKitMqttClient (C++ lib)   │
├──────────────────────────────────────┤
│  libhv (MQTT protocol via hloop)     │
└──────────────────────────────────────┘
```

## Building

### Prerequisites

- CMake 3.19+
- C++17 compiler
- The parent project (`SimpleCommKit`) must be configured first, which builds `SimpleCommKitMqttClient` and `fastmcpp`

### Build

```bash
cd SimpleCommKit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_SIMPLECOMMKITAI_FASTMCPP=ON
cmake --build . --target simplecommkitaimqttclient-fastmcpp
```

### Build with Tests

```bash
cmake .. -DSIMPLECOMMKITAIMQTTCLIENTFASTMCPP_BUILD_TESTS=ON
cmake --build . --target simplecommkitaimqttclient-fastmcpp mqtt_utility_test
ctest
```

## Usage

### MCP Client Configuration

Add to your MCP client (Claude Desktop, Cursor, etc.):

```json
{
  "mcpServers": {
    "simplecommkitaimqttclient-fastmcpp": {
      "command": "simplecommkitaimqttclient-fastmcpp"
    }
  }
}
```

### Command Line

```bash
# stdio mode (default) — used by MCP clients
simplecommkitaimqttclient-fastmcpp

# SSE mode — recommended for remote access
simplecommkitaimqttclient-fastmcpp --transport sse --host 127.0.0.1 --port 8004

# Streamable HTTP mode
simplecommkitaimqttclient-fastmcpp --transport streamable-http --port 8004

# Simple HTTP mode
simplecommkitaimqttclient-fastmcpp --transport http --port 8004
```

## Available Tools

| Tool | Description |
|------|-------------|
| `mqtt_connect` | Connect to an MQTT broker (TCP/SSL) |
| `mqtt_disconnect` | Disconnect from the broker |
| `mqtt_status` | Get connection status and broker info |
| `mqtt_publish` | Publish a message to a topic |
| `mqtt_subscribe` | Subscribe to a topic |
| `mqtt_unsubscribe` | Unsubscribe from a topic |
| `mqtt_get_messages` | Retrieve buffered messages |
| `mqtt_set_reconnect` | Configure auto-reconnection |
| `mqtt_set_will` | Set Last Will and Testament message |
| `mqtt_set_auth` | Set authentication credentials |

## Quick Start Flow

1. `mqtt_connect` → connect to an MQTT broker (e.g., `host="test.mosquitto.org"`)
2. `mqtt_status` → verify connection is established
3. `mqtt_subscribe` → subscribe to a topic (e.g., `topic="sensor/temperature"`)
4. `mqtt_publish` → publish a message (e.g., `topic="sensor/temperature"`, `data="25.5"`)
5. `mqtt_get_messages` → retrieve buffered incoming messages
6. `mqtt_disconnect` → clean up when done

## Platform Support

- **Windows**: Full support
- **macOS**: Full support
- **Linux**: Full support

## License

Apache 2.0 — See [LICENSE](../../LICENSE)
