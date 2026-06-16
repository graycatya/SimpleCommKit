# SimpleCommKitAiMqttClient

An AI-friendly MQTT client toolkit powered by **SimpleCommKitPyMqttClient**.
Connect to MQTT brokers, publish messages, and subscribe to topics from AI agents and scripts.

## Key Features

- **MCP Server**: Expose MQTT operations as tools for MCP-capable clients
- **HTTP Server**: Control MQTT connections over a REST API
- **Cross-Platform**: Works on Windows, macOS, and Linux
- **TLS Support**: Built-in SSL/TLS for secure MQTT connections

## Installation

```bash
pip install -e SimpleCommKitAi/SimpleCommKitAiMqttClient
```

## MCP Server

```json
{ "command": "simplecommkitaimqttclient-mcp" }
```

## HTTP Server

```bash
simplecommkitaimqttclient-http --host 127.0.0.1 --port 8004
```

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Health check |
| POST | `/connect` | Connect to MQTT broker |
| POST | `/disconnect` | Disconnect from broker |
| GET | `/status` | Get connection status |
| POST | `/publish` | Publish to a topic |
| POST | `/subscribe` | Subscribe to a topic |
| POST | `/unsubscribe` | Unsubscribe from a topic |
| GET | `/messages` | Get buffered messages |
| POST | `/reconnect` | Configure auto-reconnect |
| GET | `/events/stream` | SSE event stream |
