# Examples — SimpleCommKitAiTcpServerFastmcpp

## 1. Start Server and Handle Clients

```python
# Start the TCP server on port 8080
tcp_server_start(port=8080)

# Check status
status = tcp_server_status()
# → {"running": true, "connections": 0, "host": "0.0.0.0", "port": 8080}

# Client connects (from callback)...
# Client sends data...

# Retrieve messages
messages = tcp_server_get_messages()
# → {"messages": [{"client_id":1, "data_hex":"...", "data_utf8":"..."}], "count":1}

# Send response to that client
tcp_server_send(client_id=1, data="ACK")

# Stop server
tcp_server_stop()
```

## 2. Broadcast to All Clients

```python
tcp_server_start(port=9000)

# Broadcast a text message to all connected clients
tcp_server_broadcast(data="Server shutting down in 5 minutes...")

# Broadcast binary data as hex
tcp_server_broadcast(data="01 FF 00", is_hex=true)

tcp_server_stop()
```

## 3. Hex Data Exchange

```python
tcp_server_start(port=8080)

# Receive a message, respond with hex
msgs = tcp_server_get_messages()
for msg in msgs["messages"]:
    cid = msg["client_id"]
    # Echo back the hex data
    tcp_server_send(client_id=cid, data=msg["data_hex"], is_hex=true)

tcp_server_stop()
```

## 4. Echo Server

```python
tcp_server_start(port=7777)
status = tcp_server_status()
# → {"running": true, "connections": 0}

# Poll for new messages (echo them back)
msgs = tcp_server_get_messages()
for msg in msgs["messages"]:
    tcp_server_send(client_id=msg["client_id"], data=msg["data_hex"], is_hex=true)
```

## 5. Multiple Client Management

```python
tcp_server_start(port=8080)

# After clients connect and send data...
msgs = tcp_server_get_messages()

# Group messages by client
# Each message includes client_id for routing

# Send different responses to different clients
tcp_server_send(client_id=1, data="Welcome, Client 1")
tcp_server_send(client_id=2, data="Welcome, Client 2")
```

## 6. Status Monitoring

```python
# Check before starting
status = tcp_server_status()
# → {"running": false, "connections": 0}

tcp_server_start(port=8080)

# Check after starting
status = tcp_server_status()
# → {"running": true, "connections": 0, "host": "0.0.0.0", "port": 8080}

# After client connects...
status = tcp_server_status()
# → {"running": true, "connections": 1, "host": "0.0.0.0", "port": 8080}

tcp_server_stop()
```

## 7. SSE Server Mode Testing

Start the server:
```bash
simplecommkitaitcpserver-fastmcpp --transport sse --port 8007
```

Test with curl:
```bash
# Start server
curl -X POST http://127.0.0.1:8007/messages \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"tools/call","params":{"name":"tcp_server_start","arguments":{"port":8080}},"id":1}'

# Check status
curl -X POST http://127.0.0.1:8007/messages \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"tools/call","params":{"name":"tcp_server_status","arguments":{}},"id":2}'
```
