# Troubleshooting

## Common Issues

### "Failed to start WebSocket server"

1. **Port in use**: The port may already be occupied by another application. Try a different port.
   - Windows: `netstat -ano | findstr :8080`
   - Linux/macOS: `lsof -i :8080`
2. **Permission denied**: Ports below 1024 require root/administrator privileges on Unix systems. Use a port >= 1024.
3. **Address binding**: If using a specific host address, ensure the network interface is available.
4. **Firewall**: Windows Firewall may block the port. Add an exception or use `127.0.0.1` for local development.

### "WebSocket server is not running"

This error occurs when trying to send, broadcast, or check status without an active server. Always call `ws_server_start` first.

### "Failed to send data to client X"

1. **Client disconnected**: The client may have disconnected. Check `ws_server_status` for current connections.
2. **Invalid client_id**: The client ID may not exist. Client IDs are assigned dynamically by the underlying library.
3. **Send buffer full**: The underlying send buffer may be full. Check if the `bytes_sent` return value is <= 0.

### "Failed to broadcast data"

1. **No clients connected**: Broadcasting to zero clients returns 0 bytes sent. Verify with `ws_server_status`.
2. **All clients disconnected**: If all clients disconnect during broadcast, the result may still be 0.
3. **Data encoding**: For hex data, ensure the string is valid (even number of hex characters).

### Messages not being received

1. Is the `OnMessage` callback working? Check stderr logs for error messages.
2. Are you calling `ws_server_get_messages` after clients send data? Messages are buffered and must be explicitly retrieved.
3. Ensure the server is running: `ws_server_status()` should return `{"running": true}`.
4. Messages are consumed on retrieval. Calling `ws_server_get_messages` twice without new messages will return empty.

### Server is already running

If you get an error about the server already running when calling `ws_server_start`, the tool will:

1. Stop the existing server automatically
2. Clear buffered messages
3. Start a new server on the requested port

### High CPU usage

The WebSocket server uses an event-driven model and typically has low CPU usage. If you experience high CPU:

1. Check `ws_server_set_thread_num` - the default may be high. Try reducing the thread count.
2. Verify that no other processes are flooding the server with connections.

### "Max connections" setting not working

- Configuration tools (`ws_server_set_max_connections`, `ws_server_set_thread_num`) must be called BEFORE `ws_server_start`.
- Call `ws_server_set_max_connections(max_connections=50)` then `ws_server_start(port=8080)`.

## Getting Debug Output

The server writes diagnostic information to stderr. Run from terminal to see debug output:

```bash
simplecommkitaiwebsocketserver-fastmcpp 2>debug.log
```

Or run in SSE/HTTP mode and check the terminal output for error messages, client connect/disconnect events, and errors.
