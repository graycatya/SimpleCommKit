# Examples — SimpleCommKitAiUdpServerFastmcpp

## 1. Start Server and Receive Messages

```python
# Start listening on port 8002
udp_server_start(port=8002, host="0.0.0.0")

# Wait for incoming datagrams...
# Retrieve received messages
messages = udp_server_get_messages()
for msg in messages["messages"]:
    print(f"From {msg['from_host']}:{msg['from_port']} → {msg['data_utf8']}")

udp_server_stop()
```

## 2. Start and Reply to a Client

```python
udp_server_start(port=8002)

# Receive a message
result = udp_server_get_messages()
for msg in result["messages"]:
    # Reply to the sender
    udp_server_send_to(
        host=msg["from_host"],
        port=msg["from_port"],
        data="Message received!",
        is_hex=false
    )

udp_server_stop()
```

## 3. Broadcast Announcement

```python
udp_server_start(port=8002)

# Send a broadcast to all devices on the local subnet
udp_server_broadcast(data="Server is online!", is_hex=false)

udp_server_stop()
```

## 4. Send and Receive Binary Data

```python
udp_server_start(port=9000)

# Send binary protocol data to a specific client
udp_server_send_to(
    host="192.168.1.100",
    port=12345,
    data="01 02 FF 00 AB",
    is_hex=true
)

# Check for any binary responses
messages = udp_server_get_messages()
for msg in messages["messages"]:
    print(f"  hex: {msg['data_hex']}")

udp_server_stop()
```

## 5. Check Server Status

```python
status = udp_server_status()
# → {"running": true, "host": "0.0.0.0", "port": 8002}
```

## 6. Full Echo Server Pattern

```python
udp_server_start(port=8002)

# Receive all pending messages
result = udp_server_get_messages()

# Echo each message back to its sender
for msg in result["messages"]:
    udp_server_send_to(
        host=msg["from_host"],
        port=msg["from_port"],
        data=f"Echo: {msg['data_utf8']}",
        is_hex=false
    )

udp_server_stop()
```

## 7. SSE Server Mode Testing

Start the server:
```bash
simplecommkitaiupdserver-fastmcpp --transport sse --port 8102
```

Test with curl:
```bash
# Start UDP server on port 8002
curl -X POST http://127.0.0.1:8102/messages \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"tools/call","params":{"name":"udp_server_start","arguments":{"port":8002}},"id":1}'

# Broadcast a message
curl -X POST http://127.0.0.1:8102/messages \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"tools/call","params":{"name":"udp_server_broadcast","arguments":{"data":"Hello everyone"}},"id":2}'
```
