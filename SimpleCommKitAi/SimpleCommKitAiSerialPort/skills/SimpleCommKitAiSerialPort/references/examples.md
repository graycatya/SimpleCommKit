# Examples - SimpleCommKitAiSerialPort

## Basic: List ports and read data

```python
# Using MCP tools:
# 1. Call get_available_ports → find your port (e.g. "COM3")
# 2. Call open_port(port_name="COM3", baud_rate=115200)
# 3. Call read_port(port_name="COM3", size=100)
# 4. Call close_port(port_name="COM3")
```

## Write and read (command-response pattern)

```python
# 1. get_available_ports → "COM3"
# 2. open_port("COM3", baud_rate=9600)
# 3. write_port("COM3", "41540d")  # Send "AT\r" command
# 4. read_port("COM3", 256)         # Read response
# 5. close_port("COM3")
```

## Continuous monitoring

```python
# 1. get_available_ports → "/dev/ttyUSB0"
# 2. open_port("/dev/ttyUSB0", baud_rate=115200)
#    (This auto-enables read callback)
# 3. ... wait for data to arrive ...
# 4. get_buffered_data("/dev/ttyUSB0")  # Get accumulated data
# 5. ... repeat step 4 as needed ...
# 6. close_port("/dev/ttyUSB0")
```

## Change configuration

```python
# 1. open_port("COM4", baud_rate=9600)
# 2. configure_port("COM4", baud_rate=115200, flow_control=1)
# 3. port_info("COM4")  # Verify changes
# 4. close_port("COM4")
```

## Using the HTTP API directly

```bash
# List ports
curl http://127.0.0.1:8001/ports

# Open port
curl -X POST http://127.0.0.1:8001/open/COM3 \
  -H "Content-Type: application/json" \
  -d '{"baud_rate": 115200}'

# Write data (hex "Hello")
curl -X POST http://127.0.0.1:8001/port/COM3/write \
  -H "Content-Type: application/json" \
  -d '{"data": "48656c6c6f"}'

# Read data
curl -X POST "http://127.0.0.1:8001/port/COM3/read?size=100"

# Close port
curl -X POST http://127.0.0.1:8001/close/COM3
```
