# Usage Examples

## Example 1: Basic Publish and Subscribe

```
1. mqtt_connect(host="test.mosquitto.org", port=1883)
   → Connects to the public Mosquitto test broker

2. mqtt_status
   → Returns {"connected": true, "host": "test.mosquitto.org", "port": 1883}

3. mqtt_subscribe(topic="test/topic", qos=1)
   → Subscribes to test/topic with QoS 1

4. mqtt_publish(topic="test/topic", data="Hello MQTT!", qos=1)
   → Publishes a message to test/topic

5. mqtt_get_messages(topic="test/topic")
   → Retrieves the buffered message: {"topic": "test/topic", "data_utf8": "Hello MQTT!"}

6. mqtt_disconnect
   → Disconnects from the broker
```

## Example 2: Secure Connection with Authentication

```
1. mqtt_set_auth(username="myuser", password="secret123")

2. mqtt_connect(host="broker.example.com", port=8883, use_ssl=true,
                client_id="my-client-001")
   → Connects with SSL and authentication

3. mqtt_status
   → Verify the connection
```

## Example 3: Publishing Binary Data

```
# Publish binary data as hex
mqtt_publish(topic="device/raw", data="AA BB CC 01 02 FF", qos=0, is_hex=true)

# Publish a retained message
mqtt_publish(topic="device/status", data="online", qos=1, retain=true)
```

## Example 4: Will Message with Reconnection

```
1. mqtt_set_reconnect(min_delay_ms=1000, max_delay_ms=30000,
                      delay_policy=2, max_retry_cnt=5)
   → Exponential backoff, max 5 retries

2. mqtt_set_will(topic="client/status", data="offline",
                 qos=1, retain=true)

3. mqtt_connect(host="broker.local", port=1883)
   → If the client disconnects unexpectedly, the broker publishes "offline"
```

## Example 5: Multiple Subscriptions

```
# After connecting...

mqtt_subscribe(topic="sensors/temperature", qos=0)
mqtt_subscribe(topic="sensors/humidity", qos=0)
mqtt_subscribe(topic="sensors/pressure", qos=0)

# Wait for data to arrive, then retrieve all at once
mqtt_get_messages
→ Returns messages from all three topics

# Or retrieve just temperature
mqtt_get_messages(topic="sensors/temperature")
→ Returns only temperature messages
```

## Example 6: IoT Sensor Monitoring

```
1. mqtt_connect(host="my-broker.local", port=1883)

2. mqtt_subscribe(topic="factory/machine1/status", qos=1)
3. mqtt_subscribe(topic="factory/machine1/temperature", qos=0)

4. # Periodically check for new messages
   mqtt_get_messages

5. # Send a command
   mqtt_publish(topic="factory/machine1/command",
                data="RESTART", qos=1)

6. mqtt_disconnect
```
