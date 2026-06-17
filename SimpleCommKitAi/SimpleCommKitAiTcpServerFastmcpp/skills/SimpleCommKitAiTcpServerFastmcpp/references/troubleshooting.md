# Troubleshooting — SimpleCommKitAiTcpServerFastmcpp

## Common Issues

### "Server start failed"

**Cause:** The port is already in use, or binding is not permitted.

**Solutions:**
1. Check if another process is using the port: `netstat -an | grep <port>` (Linux/macOS) or `netstat -an | findstr <port>` (Windows)
2. Try a different port number
3. On Linux, ports < 1024 require root privileges
4. Ensure the host address is valid (e.g., `0.0.0.0` for all interfaces)

### "Server is already running"

**Cause:** `tcp_server_start()` called while a server instance is already listening.

**Solutions:**
1. Call `tcp_server_stop()` first
2. Check status with `tcp_server_status()` before starting

### "Send to client X failed"

**Cause:** The specified client ID does not exist or has disconnected.

**Solutions:**
1. Client IDs come from `tcp_server_get_messages()` — verify the client is still connected
2. Check `tcp_server_status()` for current connection count
3. The client may have disconnected between receiving a message and sending a response

### "Broadcast failed"

**Cause:** No clients are connected, or server is not running.

**Solutions:**
1. Verify server is running with `tcp_server_status()`
2. Check that at least one client is connected
3. Some broadcast implementations return 0 if no clients are connected

### No Messages Received

**Cause:** No clients have sent data, or messages were already consumed.

**Solutions:**
1. `tcp_server_get_messages()` clears the buffer — each message is returned only once
2. Verify clients are actually sending data
3. Check that clients are connecting to the correct host and port

### "Server not initialized"

**Cause:** `tcp_server_send`, `tcp_server_broadcast`, or `tcp_server_stop` called before `tcp_server_start`.

**Solutions:**
1. Always call `tcp_server_start()` first
2. Use `tcp_server_status()` to check if the server is ready

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
- The tool uses `simplecommkitaitcpserver-fastmcpp.exe` as the executable name
- Firewall may prompt for network access on first run
- Use `Ctrl+Z` followed by Enter to send EOF in stdio mode

### Linux
- Ports < 1024 require `sudo` or `CAP_NET_BIND_SERVICE` capability
- Use `Ctrl+D` to send EOF in stdio mode

### macOS
- Standard BSD socket behavior
- Use `Ctrl+D` to send EOF in stdio mode

## Getting Debug Output

The server writes log messages to stderr:

```
[SimpleCommKitAiTcpServerFastmcpp] Client connected: 1
[SimpleCommKitAiTcpServerFastmcpp] Client disconnected: 1
[SimpleCommKitAiTcpServerFastmcpp] Error: 123 - Connection timeout
```

To capture debug logs:
```bash
# Redirect stderr to a file
simplecommkitaitcpserver-fastmcpp --transport http 2> debug.log

# Or view in real-time (Unix)
simplecommkitaitcpserver-fastmcpp --transport http 2>&1 | grep SimpleCommKit
```
