# Usage Examples

## Example 1: Start Server and Handle Client

```
1. ws_server_start(port=8080, host="0.0.0.0")
   → Starts WebSocket server on port 8080

2. ws_server_status()
   → Returns {"running": true, "connections": 0, "host": "0.0.0.0", "port": 8080}

# Wait for a client to connect, then check again
3. ws_server_status()
   → {"running": true, "connections": 1, ...}

4. ws_server_stop()
   → Stops the server and disconnects all clients
```

## Example 2: Broadcast to All Clients

```
ws_server_start(port=9000)

# Broadcast a welcome message as text
ws_server_broadcast(data="Welcome to the server!", is_hex=false)
→ {"bytes_sent": 22, "clients": 3}

ws_server_stop()
```

## Example 3: Send to a Specific Client

```
ws_server_start(port=9000)

ws_server_status()
→ {"connections": 2}  # clients 1 and 2

ws_server_send(client_id=1, data="Hello client 1")
→ {"client_id": 1, "bytes_sent": 14}

ws_server_send(client_id=2, data="Hello client 2")
→ {"client_id": 2, "bytes_sent": 14}

ws_server_stop()
```

## Example 4: Receive Messages from Clients

```
ws_server_start(port=9000)

# After clients send messages, retrieve them
ws_server_get_messages()
→ Returns messages tagged with client_id, data_hex, data_utf8

# Messages are consumed (cleared) after retrieval
ws_server_get_messages()
→ Returns empty (unless new messages arrived)

ws_server_stop()
```

## Example 5: Binary Data Exchange

```
ws_server_start(port=9000)

# Broadcast hex-encoded binary data
ws_server_broadcast(data="01020304FF", is_hex=true)
→ {"bytes_sent": 5, "clients": 2}

# Send hex data to specific client
ws_server_send(client_id=1, data="AB CD EF", is_hex=true)

ws_server_stop()
```

## Example 6: Echo Server Pattern

```
ws_server_start(port=8080)

# In a real scenario, the AI agent polls for messages:
# 1. Get client messages
ws_server_get_messages()
→ [{"client_id": 1, "data_utf8": "ping", "data_hex": "70696e67"}]

# 2. Echo back to the same client
ws_server_send(client_id=1, data="ping")

# 3. Repeat
ws_server_get_messages()
ws_server_stop()
```

## Example 7: Configure Before Start

```
# Set max connections and thread count
ws_server_set_max_connections(max_connections=50)
ws_server_set_thread_num(thread_num=4)

# Start with configuration
ws_server_start(port=8080)

ws_server_status()
→ Verify server is running

ws_server_stop()
```

## Example 8: Chat Server

```
ws_server_start(port=9000)

# Send system message
ws_server_broadcast(data="Chat server is online!", is_hex=false)

# Forward messages between clients by polling
messages = ws_server_get_messages()
for each message:
    # Broadcast to all other clients
    other_clients = [...]
    for client_id in other_clients:
        ws_server_send(client_id, message["data_utf8"])

ws_server_stop()
```
