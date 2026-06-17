# Troubleshooting — SimpleCommKitAiTcpClientFastmcpp

## Common Issues

### "Failed to connect"

**Cause:** The target server is not running or unreachable.

**Solutions:**
1. Verify the server is running: `telnet <host> <port>` or `nc -zv <host> <port>`
2. Check that hostname resolves: `ping <host>`
3. Ensure firewall allows the connection
4. Verify the port number is correct

### "TCP client is not connected"

**Cause:** `tcp_send` or `tcp_get_messages` called before `tcp_connect`.

**Solutions:**
1. Always call `tcp_connect()` first
2. Check connection status with `tcp_status()`
3. If reconnecting, the client may have been disconnected — check status first

### "Failed to send data"

**Cause:** Connection dropped or network error.

**Solutions:**
1. Check `tcp_status()` to verify connection
2. The server may have closed the connection
3. Enable auto-reconnect with `tcp_set_reconnect()`
4. Reconnect with `tcp_connect()`

### No Messages Received

**Cause:** Server hasn't sent data, or message was consumed already.

**Solutions:**
1. Verify the server is sending data in response to your request
2. `tcp_get_messages()` clears the buffer — each message is returned only once
3. Check that you're connecting to the correct host/port
4. If using a request-response protocol, send a request first

### Connection Drops Unexpectedly

**Cause:** Network instability, server timeout, or server crash.

**Solutions:**
1. Enable auto-reconnect with `tcp_set_reconnect()` for resilience
2. Check server logs for disconnection reasons
3. Verify network stability between client and server
4. Some firewalls/NATs drop idle connections — consider sending periodic keep-alive

### Hex Data Issues

**Cause:** Invalid hex string format.

**Solutions:**
1. Ensure hex string has even number of characters: `"0A1B"` (4 chars → 2 bytes)
2. Only valid hex characters: `0-9`, `a-f`, `A-F`
3. Whitespace between bytes is optional and ignored: `"0A 1B"` is valid
4. Odd-length strings like `"ABC"` will throw an error

### Server Exits Immediately (stdio mode)

**Cause:** No input is being sent to the stdio server.

**Solution:** In stdio mode, the server blocks waiting for JSON-RPC requests on stdin.
It is designed to be launched by an MCP client process that communicates via stdin/stdout.

## Platform-Specific Notes

### Windows
- The tool uses `simplecommkitaitcpclient-fastmcpp.exe` as the executable name
- Firewall may prompt for network access on first run
- Use `Ctrl+Z` followed by Enter to send EOF in stdio mode

### Linux
- May need `CAP_NET_RAW` or similar capabilities for certain ports
- Use `Ctrl+D` to send EOF in stdio mode

### macOS
- Standard BSD socket behavior
- Use `Ctrl+D` to send EOF in stdio mode

## Getting Debug Output

The server writes log messages to stderr (not stdout, which is reserved for MCP protocol):

```
[SimpleCommKitAiTcpClientFastmcpp] Connected to server.
[SimpleCommKitAiTcpClientFastmcpp] Disconnected from server.
[SimpleCommKitAiTcpClientFastmcpp] Error: 123 - Connection timeout
```

To capture debug logs:
```bash
# Redirect stderr to a file
simplecommkitaitcpclient-fastmcpp --transport http 2> debug.log

# Or view in real-time (Unix)
simplecommkitaitcpclient-fastmcpp --transport http 2>&1 | grep SimpleCommKit
```
