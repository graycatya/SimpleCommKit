# Troubleshooting — SimpleCommKitAiUdpClientFastmcpp

## Common Issues

### "UDP socket is not open"

**Cause:** `udp_send_to` or `udp_get_messages` called before `udp_open`.

**Solutions:**
1. Always call `udp_open()` first
2. Check socket status with `udp_status()`
3. If you closed the socket earlier, re-open it

### "Failed to open UDP socket"

**Cause:** The requested local port is already in use, or the host address is invalid.

**Solutions:**
1. Use `local_port=0` to let the OS assign an available port
2. Check if another application is using the port: `netstat -an | grep <port>`
3. Ensure the local host address is valid for your network interface
4. On Linux, ports below 1024 may require root privileges

### "Failed to send UDP datagram"

**Cause:** Network error, invalid target address, or socket closed.

**Solutions:**
1. Check that the target host is reachable: `ping <host>`
2. Verify the target port is correct and a service is listening
3. Ensure the socket is open with `udp_status()`
4. UDP is best-effort — no delivery guarantee. The destination may not respond

### No Datagrams Received

**Cause:** No data sent to your socket, or messages were already consumed.

**Solutions:**
1. `udp_get_messages()` clears the buffer — each message is returned only once
2. Ensure the sender knows your local port (use `udp_status()` to check the opened port)
3. Verify no firewall is blocking incoming UDP on your local port
4. Some NAT configurations may require the socket to send first before receiving
5. UDP doesn't require a "connection" — make sure the sender is targeting the correct host:port

### Socket Binds to Port 0

**Cause:** `udp_open()` was called with `local_port=0`, which tells the OS to choose a port.

**Note:** This is normal behavior. The OS assigns an available ephemeral port.
If you need a specific local port for receiving, specify it explicitly.

### Hex Data Issues

**Cause:** Invalid hex string format.

**Solutions:**
1. Ensure hex string has even number of characters: `"0A1B"` (4 chars → 2 bytes)
2. Only valid hex characters: `0-9`, `a-f`, `A-F`
3. Whitespace between bytes is optional and ignored: `"0A 1B"` is valid
4. Odd-length strings like `"ABC"` will throw an error

### UDP Datagram Size Limits

**Cause:** Datagrams exceeding the maximum UDP size may be fragmented or dropped.

**Note:** The theoretical maximum UDP datagram size is 65,507 bytes (65,535 − 8-byte UDP header − 20-byte IP header).
In practice, many networks have a lower MTU (typically ~1500 bytes for Ethernet).
For large data, consider splitting into smaller datagrams or using TCP.

### Server Exits Immediately (stdio mode)

**Cause:** No input is being sent to the stdio server.

**Solution:** In stdio mode, the server blocks waiting for JSON-RPC requests on stdin.
It is designed to be launched by an MCP client process that communicates via stdin/stdout.

## Platform-Specific Notes

### Windows
- The tool uses `simplecommkitaiudpclient-fastmcpp.exe` as the executable name
- Firewall may prompt for network access on first run
- Use `Ctrl+Z` followed by Enter to send EOF in stdio mode

### Linux
- Ports below 1024 require root or `CAP_NET_BIND_SERVICE` capability
- Use `Ctrl+D` to send EOF in stdio mode
- Check UDP buffer sizes: `sysctl net.core.rmem_default net.core.wmem_default`

### macOS
- Standard BSD socket behavior
- Use `Ctrl+D` to send EOF in stdio mode

## Getting Debug Output

The server writes log messages to stderr (not stdout, which is reserved for MCP protocol):

```
[SimpleCommKitAiUdpClientFastmcpp] Error: 123 - Connection timeout
```

To capture debug logs:
```bash
# Redirect stderr to a file
simplecommkitaiudpclient-fastmcpp --transport http 2> debug.log

# Or view in real-time (Unix)
simplecommkitaiudpclient-fastmcpp --transport http 2>&1 | grep SimpleCommKit
```
