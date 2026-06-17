# Examples — SimpleCommKitAiTcpClientFastmcpp

## 1. Connect and Send Text

```python
# Connect to a local server
tcp_connect(host="127.0.0.1", port=8080)

# Send a text message
tcp_send(data="Hello, TCP Server!", is_hex=false)

# Check status
tcp_status()
# → {"connected": true}

# Disconnect
tcp_disconnect()
```

## 2. Connect and Send Binary Data (Hex)

```python
tcp_connect(host="192.168.1.100", port=9000)

# Send binary protocol data as hex
tcp_send(data="01 02 FF 00 AB", is_hex=true)

tcp_disconnect()
```

## 3. Receive Messages

```python
tcp_connect(host="127.0.0.1", port=8080)

# Send a request
tcp_send(data="PING")

# Wait a moment, then retrieve any responses
messages = tcp_get_messages()
# → {"messages": [{"data_hex": "504F4E47", "data_utf8": "PONG", "data_length": 4}], "count": 1}

tcp_disconnect()
```

## 4. Auto-Reconnection

```python
# Configure auto-reconnect before connecting
tcp_set_reconnect(
    min_delay_ms=500,
    max_delay_ms=5000,
    delay_policy=2,     # exponential backoff
    max_retry_cnt=5     # retry up to 5 times
)

tcp_connect(host="127.0.0.1", port=8080)
# Client will auto-reconnect if the server restarts
```

## 5. Check Status Only

```python
status = tcp_status()
print(status)  # {"connected": false}
```

## 6. Full Request-Response Cycle

```python
# Connect
tcp_connect(host="api.example.com", port=9001)

# Send a JSON request
tcp_send(data='{"method":"get_status","id":1}\n', is_hex=false)

# Retrieve the response
response = tcp_get_messages()
for msg in response["messages"]:
    print(f"  hex: {msg['data_hex']}")
    print(f"  text: {msg['data_utf8']}")

# Disconnect
tcp_disconnect()
```

## 7. SSE Server Mode Testing

Start the server:
```bash
simplecommkitaitcpclient-fastmcpp --transport sse --port 8006
```

Test with curl:
```bash
# Connect (via MCP tools/list then tools/call)
curl -X POST http://127.0.0.1:8006/messages \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"tools/call","params":{"name":"tcp_connect","arguments":{"host":"127.0.0.1","port":8080}},"id":1}'
```
