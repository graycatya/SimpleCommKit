# Usage Examples

## Example 1: Discover and Connect to a BLE Device

```
1. get_adapters
   → Returns available Bluetooth adapters

2. scan_for(timeout_ms=5000)
   → Returns list of nearby BLE peripherals with addresses and RSSI

3. connect(address="AA:BB:CC:DD:EE:FF")
   → Connects to the specified peripheral

4. services(address="AA:BB:CC:DD:EE:FF")
   → Returns all GATT services and characteristics

5. disconnect(address="AA:BB:CC:DD:EE:FF")
   → Disconnects when done
```

## Example 2: Read Device Information

```
# After connecting to a device
read(address="AA:BB:CC:DD:EE:FF",
     service_uuid="180a",
     characteristic_uuid="2a29")
→ Returns manufacturer name as data_hex and data_utf8

read(address="AA:BB:CC:DD:EE:FF",
     service_uuid="180a",
     characteristic_uuid="2a24")
→ Returns model number
```

## Example 3: Read Battery Level

```
read(address="AA:BB:CC:DD:EE:FF",
     service_uuid="180f",
     characteristic_uuid="2a19")
→ Returns battery percentage (0-100) as a single byte
```

## Example 4: Subscribe to Notifications

```
# Heart Rate Monitor example
notify(address="AA:BB:CC:DD:EE:FF",
       service_uuid="180d",
       characteristic_uuid="2a37")
→ Subscribes to heart rate measurement notifications

# Wait for data to accumulate, then retrieve
get_notifications(address="AA:BB:CC:DD:EE:FF")
→ Returns buffered heart rate data

# Clean up
unsubscribe(address="AA:BB:CC:DD:EE:FF",
            service_uuid="180d",
            characteristic_uuid="2a37")
```

## Example 5: Write to a Characteristic

```
# Write 01 to turn on an LED (example)
write_command(address="AA:BB:CC:DD:EE:FF",
              service_uuid="ffe0",
              characteristic_uuid="ffe1",
              data="01")

# Write with acknowledgment
write_request(address="AA:BB:CC:DD:EE:FF",
              service_uuid="ffe0",
              characteristic_uuid="ffe1",
              data="00FF")
```

## Example 6: Read and Write Descriptors

```
# Read CCCD (Client Characteristic Configuration Descriptor)
descriptor_read(address="AA:BB:CC:DD:EE:FF",
                service_uuid="180d",
                characteristic_uuid="2a37",
                descriptor_uuid="2902")

# Write CCCD to enable notifications
descriptor_write(address="AA:BB:CC:DD:EE:FF",
                 service_uuid="180d",
                 characteristic_uuid="2a37",
                 descriptor_uuid="2902",
                 data="0100")
```

## Example 7: Check Bluetooth and List Connected Devices

```
bluetooth_enabled
→ Returns true/false

connected_devices
→ Returns list of currently connected peripherals
```
