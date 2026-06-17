# Troubleshooting

## Common Issues

### "Failed to open WebSocket connection"

1. **Check the URL**: Ensure the WebSocket URL is correct and uses `ws://` or `wss://`.
2. **Server not running**: Verify the target WebSocket server is running and listening on the specified port.
3. **Network issues**: Check firewall settings, network connectivity, and that the host/port is reachable.
4. **TLS errors**: For `wss://` URLs, ensure the server's TLS certificate is valid. Self-signed certificates may require additional configuration.

### "WebSocket client is not connected"

This error occurs when trying to send data without an active connection. Always call `ws_connect` first and verify with `ws_status`.

### Connection drops unexpectedly

1. **Network instability**: Unstable network connections can cause WebSocket disconnections.
2. **Server-side timeout**: The server may have an idle timeout. Use `ws_set_ping_interval` to keep the connection alive.
3. **Server restart**: If the server restarts, enable auto-reconnect with `ws_set_reconnect`.
4. **Firewall/NAT**: Some network configurations may drop idle connections.

### Messages not being received

1. Is the `OnMessage` callback working? Check stderr logs for any error messages.
2. Are you calling `ws_get_messages` after data is sent? Messages are buffered and must be explicitly retrieved.
3. Ensure you are connected: `ws_status()` should return `{"connected": true}`.
4. The server may not be sending a response to your particular message.

### TLS / wss:// issues

1. **Certificate verification**: The underlying library may verify server certificates. For development with self-signed certs, TLS settings may need adjustment.
2. **Protocol mismatch**: Ensure the server supports the WebSocket protocol (not raw TCP or HTTP).
3. **Port**: Standard wss port is 443, but custom ports are supported.

### "Failed to send data"

1. The send buffer may be full. Check the `bytes_sent` return value.
2. If `bytes_sent <= 0`, the connection may have been lost. Check `ws_status()`.
3. For hex data, ensure the hex string is valid (even number of characters, only 0-9, a-f, A-F).

### High latency in message delivery

1. **Polling interval**: Messages are buffered and retrieved on-demand via `ws_get_messages`. If you need real-time delivery, call `ws_get_messages` more frequently.
2. **Network latency**: Physical network distance can add latency.
3. **Server processing**: The WebSocket server may have its own processing delays.

## Getting Debug Output

The server writes diagnostic information to stderr. Run from terminal to see debug output:

```bash
simplecommkitaiwebsocketclient-fastmcpp 2>debug.log
```

Or run in SSE/HTTP mode and check the terminal output for error messages.
