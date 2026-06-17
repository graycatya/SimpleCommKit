# Troubleshooting — SimpleCommKitAiUdpServerFastmcpp

## Common Issues

### "Failed to start UDP server"

**Cause:** The requested port is already in use, or the host address is invalid.

**Solutions:**
1. Check if another application is using the port: `netstat -an | grep <port>`
2. Try a different port number
3. Ensure the host address is valid for your network interface
4. On Linux, ports below 1024 may require root privileges

### "UDP server is not running"

**Cause:** `udp_server_send_to`, `udp_server_broadcast`, or `udp_server_get_messages` called before starting.

**Solutions:**
1. Always call `udp_server_start()` first
2. Check server status with `udp_server_status()`
3. The server may have been stopped — re-start it

### "Failed to send UDP datagram"

**Cause:** Network error, invalid target address, or server not running.

**Solutions:**
1. Verify the target host is reachable: `ping <host>`
2. Check that the server is running with `udp_server_status()`
3. UDP is best-effort — no delivery guarantee. The destination may not be listening

### "Failed to broadcast" / "no clients reachable"

**Cause:** Network configuration prevents broadcast, or no listeners on the subnet.

**Solutions:**
1. Broadcast (255.255.255.255) only reaches the local subnet
2. Some routers/firewalls may block broadcast packets
3. Ensure target clients are listening on the correct port
4. Verify the server's network interface is up

### No Datagrams Received

**Cause:** No sender is targeting this server, or messages were already consumed.

**Solutions:**
1. `udp_server_get_messages()` clears the buffer — each message is returned only once
2. Ensure senders are targeting the correct host:port that the server is listening on
3. Verify no firewall is blocking incoming UDP on the server's port
4. Check that the server is bound to the correct interface (`host="0.0.0.0"` listens on all)

### Client Can't Reach Server

**Cause:** The server is bound to localhost (127.0.0.1) or a specific interface.

**Solutions:**
1. Use `host="0.0.0.0"` to listen on all network interfaces
2. If binding to a specific IP, ensure clients use that same IP
3. Check firewall rules on the server machine
4. Verify the correct port number on the client side

### Hex Data Issues

**Cause:** Invalid hex string format.

**Solutions:**
1. Ensure hex string has even number of characters: `"0A1B"` (4 chars → 2 bytes)
2. Only valid hex characters: `0-9`, `a-f`, `A-F`
3. Whitespace between bytes is optional and ignored: `"0A 1B"` is valid
4. Odd-length strings like `"ABC"` will throw an error

### UDP Datagram Size Limits

**Cause:** Datagrams exceeding the maximum UDP size may be fragmented or dropped.

**Note:** The theoretical maximum UDP datagram size is 65,507 bytes.
In practice, networks have a lower MTU (~1500 bytes for Ethernet).
For large data, consider splitting into smaller datagrams.

### Server Exits Immediately (stdio mode)

**Cause:** No input is being sent to the stdio server.

**Solution:** In stdio mode, the server blocks waiting for JSON-RPC requests on stdin.
It is designed to be launched by an MCP client process that communicates via stdin/stdout.

## Platform-Specific Notes

### Windows
- The tool uses `simplecommkitaiupdserver-fastmcpp.exe` as the executable name
- Firewall may prompt for network access on first run
- Use `Ctrl+Z` followed by Enter to send EOF in stdio mode

### Linux
- Ports below 1024 require root or `CAP_NET_BIND_SERVICE` capability
- Use `Ctrl+D` to send EOF in stdio mode
- For broadcast: ensure network interface supports broadcast

### macOS
- Standard BSD socket behavior
- Use `Ctrl+D` to send EOF in stdio mode

## Getting Debug Output

The server writes log messages to stderr (not stdout, which is reserved for MCP protocol):

```
[SimpleCommKitAiUdpServerFastmcpp] Error: 123 - Connection timeout
```

To capture debug logs:
```bash
# Redirect stderr to a file
simplecommkitaiupdserver-fastmcpp --transport http 2> debug.log

# Or view in real-time (Unix)
simplecommkitaiupdserver-fastmcpp --transport http 2>&1 | grep SimpleCommKit
```
