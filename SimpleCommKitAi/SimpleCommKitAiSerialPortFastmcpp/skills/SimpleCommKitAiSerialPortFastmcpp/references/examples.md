# SimpleCommKitAiSerialPortFastmcpp - Usage Examples

## Example: Using with MCP Client (Claude Desktop / Cursor)

Add to your MCP client configuration:

```json
{
  "mcpServers": {
    "simplecommkitaiserialport-fastmcpp": {
      "command": "simplecommkitaiserialport-fastmcpp"
    }
  }
}
```

Then prompt your AI agent:

> List all serial ports on my computer.

> Open COM3 at 115200 baud and write "Hello" to it.

> Open /dev/ttyUSB0 at 9600 baud, read 100 bytes, and show me the data.

## Example: Running as SSE Server

```bash
# Start SSE server on port 8005
simplecommkitaiserialport-fastmcpp --transport sse --port 8005

# In another terminal, test with curl:

# List ports
curl -X POST "http://127.0.0.1:8005/messages" \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_available_ports","arguments":{}}}'

# Open a port
curl -X POST "http://127.0.0.1:8005/messages" \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"open","arguments":{"port_name":"COM3","baud_rate":115200}}}'

# Write data
curl -X POST "http://127.0.0.1:8005/messages" \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"write","arguments":{"port_name":"COM3","data":"48656C6C6F"}}}'
```

## Example: Typical Serial Communication Flow

1. List ports:
   ```
   get_available_ports()
   ```

2. Open with desired settings:
   ```
   open(port_name="COM3", baud_rate=115200, parity="none", data_bits=8, stop_bits="one")
   ```

3. Write hex data:
   ```
   write(port_name="COM3", data="41 54 0D 0A")   # "AT\r\n"
   ```

4. Read response:
   ```
   read(port_name="COM3", size=256)
   # or for async buffered data:
   get_read_data(port_name="COM3")
   ```

5. Close when done:
   ```
   close(port_name="COM3")
   ```

## Example: Configuring Different Settings

```json
// Open with 9600 baud, even parity, hardware flow control
{
  "port_name": "/dev/ttyUSB0",
  "baud_rate": 9600,
  "parity": "even",
  "stop_bits": "one",
  "flow_control": "hardware"
}
```

## Example: Dynamic Reconfiguration

```json
// Check current config
{ "port_name": "COM3" }  // get_config

// Change baud rate only
{ "port_name": "COM3", "baud_rate": 38400 }  // set_config

// Change multiple settings
{
  "port_name": "COM3",
  "baud_rate": 57600,
  "parity": "odd",
  "flow_control": "software"
}  // set_config
```

## Example: Hardware Flow Control

```json
// Open with DTR/RTS control
open(port_name="COM3", flow_control="hardware")

// Manually assert DTR
set_dtr(port_name="COM3", set=true)

// De-assert RTS
set_rts(port_name="COM3", set=false)
```

## Example: Flushing Buffers

```json
// Clear stale data before sending a command
flush_buffers(port_name="COM3", direction="both")

// Write a command
write(port_name="COM3", data="41540D0A")

// Read the response
read_all(port_name="COM3")
```

## Example: Error Handling

```
// After a failed operation, check what went wrong
get_error(port_name="COM3")

// Returns something like:
{
  "port_name": "COM3",
  "error_code": 4,
  "error_message": "...",
  "error_description": "Port not open"
}
```

## Example: C++ Client Integration

```cpp
#include <fastmcpp/client/client.hpp>
#include <fastmcpp/client/transports.hpp>

// Connect to the MCP server via stdio
fastmcpp::client::StdioTransport transport("simplecommkitaiserialport-fastmcpp");
fastmcpp::client::Client client(transport);

// Initialize
client.initialize();

// Call get_available_ports
auto ports = client.call_tool("get_available_ports", {});

// Open a port
nlohmann::json args;
args["port_name"] = "COM3";
args["baud_rate"] = 115200;
auto open_result = client.call_tool("open", args);

// Write data
args.clear();
args["port_name"] = "COM3";
args["data"] = "48656C6C6F";
client.call_tool("write", args);

// Close
args.clear();
args["port_name"] = "COM3";
client.call_tool("close", args);
```
