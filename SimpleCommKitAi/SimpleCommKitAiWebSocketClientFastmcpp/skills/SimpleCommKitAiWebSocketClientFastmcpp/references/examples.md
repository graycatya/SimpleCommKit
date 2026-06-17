# Usage Examples

## Example 1: Connect and Send a Message

```
1. ws_connect(url="ws://127.0.0.1:8080/ws")
   → Opens a WebSocket connection

2. ws_status()
   → Returns {"connected": true}

3. ws_send(data="Hello, WebSocket!", is_hex=false)
   → Sends text message, returns {"bytes_sent": 17}

4. ws_disconnect()
   → Closes the connection
```

## Example 2: Binary Data Exchange

```
# Connect and send hex bytes
ws_connect(url="ws://127.0.0.1:9000/ws")

ws_send(data="01020304FF", is_hex=true)
→ Returns {"bytes_sent": 5}

# Retrieve any server response
ws_get_messages()
→ Returns buffered messages with hex and UTF-8 representations

ws_disconnect()
```

## Example 3: Echo Server Interaction

```
ws_connect(url="wss://echo.websocket.org")

ws_send(data="Hello, echo server!", is_hex=false)

# Wait briefly for response, then check
ws_get_messages()
→ Returns the echoed message

ws_disconnect()
```

## Example 4: Configure Auto-Reconnect

```
# Set up reconnect before connecting
ws_set_reconnect(
    min_delay_ms=1000,
    max_delay_ms=5000,
    delay_policy=2,
    max_retry_cnt=0
)

# Connect — will auto-reconnect on failure
ws_connect(url="ws://127.0.0.1:8080/ws")

# Messages received after reconnect will be buffered
ws_get_messages()

ws_disconnect()
```

## Example 5: Keep-Alive with Ping

```
ws_connect(url="ws://127.0.0.1:8080/ws")

# Send a ping every 30 seconds to keep connection alive
ws_set_ping_interval(interval_ms=30000)

ws_send(data="Heartbeat on", is_hex=false)

# Later, disable pings
ws_set_ping_interval(interval_ms=0)

ws_disconnect()
```

## Example 6: Connection Timeout

```
# Set a short timeout for local connections
ws_set_connect_timeout(timeout_ms=2000)

ws_connect(url="ws://127.0.0.1:8080/ws")

ws_status()
→ Check if connected

ws_disconnect()
```

## Example 7: Send and Receive JSON Messages

```
ws_connect(url="ws://127.0.0.1:8080/ws")

# Send a JSON command as text
ws_send(data='{"action":"ping","timestamp":1234567890}', is_hex=false)

# Retrieve the JSON response
ws_get_messages()
→ Returns the server's JSON response in data_utf8

ws_disconnect()
```

## Example 8: Secure WebSocket (wss://)

```
# Connect to a TLS-secured WebSocket server
ws_connect(url="wss://example.com/socket")

ws_send(data="Secure message", is_hex=false)

ws_get_messages()

ws_disconnect()
```
