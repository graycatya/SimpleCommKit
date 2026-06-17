# Examples — SimpleCommKitAiUdpClientFastmcpp

## 1. Open Socket and Send Text

```python
# Open a local UDP socket (OS assigns port)
udp_open(local_port=0, local_host="0.0.0.0")

# Send a text datagram
udp_send_to(host="127.0.0.1", port=8002, data="Hello, UDP Server!", is_hex=false)

# Check status
udp_status()
# → {"is_open": true}

# Close
udp_close()
```

## 2. Open Socket and Send Binary Data (Hex)

```python
udp_open()

# Send binary protocol data as hex
udp_send_to(host="192.168.1.100", port=9000, data="01 02 FF 00 AB", is_hex=true)

udp_close()
```

## 3. Receive Datagrams

```python
udp_open(local_port=8003)

# Send a request
udp_send_to(host="127.0.0.1", port=8002, data="PING")

# Wait a moment, then retrieve any responses
messages = udp_get_messages()
# → {"messages": [{"data_hex": "504F4E47", "data_utf8": "PONG", "data_length": 4}], "count": 1}

udp_close()
```

## 4. Send to Multiple Destinations

```python
udp_open()

# UDP is connectionless — send to different destinations
udp_send_to(host="10.0.0.1", port=9000, data="Probe")
udp_send_to(host="10.0.0.2", port=9000, data="Probe")
udp_send_to(host="10.0.0.3", port=9000, data="Probe")

udp_close()
```

## 5. Check Socket Status Only

```python
status = udp_status()
# → {"is_open": false}
```

## 6. Full Send-Receive Cycle

```python
# Open socket on a specific port
udp_open(local_port=12345)

# Send a JSON request
udp_send_to(host="127.0.0.1", port=8002, data='{"command":"status"}\n', is_hex=false)

# Retrieve the response
response = udp_get_messages()
for msg in response["messages"]:
    print(f"  hex: {msg['data_hex']}")
    print(f"  text: {msg['data_utf8']}")

# Close
udp_close()
```

## 7. SSE Server Mode Testing

Start the server:
```bash
simplecommkitaiudpclient-fastmcpp --transport sse --port 8008
```

Test with curl:
```bash
# Open socket (via MCP tools/call)
curl -X POST http://127.0.0.1:8008/messages \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"tools/call","params":{"name":"udp_open","arguments":{"local_port":0}},"id":1}'

# Send a datagram
curl -X POST http://127.0.0.1:8008/messages \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"tools/call","params":{"name":"udp_send_to","arguments":{"host":"127.0.0.1","port":8002,"data":"Hello"}},"id":2}'
```
