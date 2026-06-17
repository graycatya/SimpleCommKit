# Troubleshooting

## Common Issues

### "Not connected" error

This means you haven't called `mqtt_connect` or the connection has been lost.

1. Call `mqtt_connect` with the broker host and port.
2. Use `mqtt_status` to check the current connection state.
3. If the connection dropped, reconnect.
4. Enable reconnection with `mqtt_set_reconnect` before connecting to auto-recover.

### Connection fails immediately

1. Verify the broker host is reachable: `ping broker.example.com`
2. Check the port number: default is 1883 for plain TCP, 8883 for SSL
3. Check firewall settings — the MQTT port must be open
4. If using SSL, verify certificates are correct
5. Try connecting to a test broker first: `test.mosquitto.org`

### SSL/TLS connection fails

1. Verify `use_ssl=true` is set in `mqtt_connect`
2. Check that the broker supports SSL on the specified port
3. Certificate verification may fail for self-signed certificates
4. Try a plain connection first to isolate the issue

### Authentication fails

1. Use `mqtt_set_auth` before `mqtt_connect`
2. Verify username and password are correct
3. Some brokers require specific client IDs
4. Call `mqtt_status` after connecting — if `connected=false`, auth likely failed

### Messages not being received

1. Verify you are subscribed to the correct topic with `mqtt_subscribe`
2. Check `mqtt_status` — messages arrive only while connected
3. Topic names are case-sensitive
4. Try subscribing with a higher QoS level
5. Check if another client is publishing to the expected topic
6. The message may be retained by the broker but not delivered because you subscribed after publishing. Use `retain=true` when publishing for late subscribers.

### Publish fails with error code

1. Verify you are connected: `mqtt_status`
2. Check the QoS level — some brokers restrict QoS 2
3. For binary data, ensure `is_hex=true` and the hex string is valid
4. Topic name must not be empty
5. Some brokers have topic restrictions (e.g., require specific prefixes)

### Subscriptions lost after reconnect

If automatic reconnection is enabled, subscriptions must be re-established after reconnection. Use `mqtt_set_reconnect` before connecting (the failed connection is dropped, and you need to reconnect and re-subscribe manually in the current implementation).

### "MQTT client not available" warning

This warning means the `SimpleCommKitMqttClient` C++ library failed to initialize. Common causes:
- Missing libhv dependency
- The parent SimpleCommKit project was not built with MQTT support (`WITH_MQTT=ON` is required)

## Getting Debug Output

The server writes diagnostic information to stderr. Run from terminal to see debug output:

```bash
simplecommkitaimqttclient-fastmcpp 2>debug.log
```

Or run in SSE/HTTP mode and check the terminal output for error messages.
