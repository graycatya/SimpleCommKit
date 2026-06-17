# SimpleCommKitAiHidFastmcpp - Usage Examples

## Example: Using with MCP Client (Claude Desktop / Cursor)

Add to your MCP client configuration:

```json
{
  "mcpServers": {
    "simplecommkitaihid-fastmcpp": {
      "command": "simplecommkitaihid-fastmcpp"
    }
  }
}
```

Then prompt your AI agent:

> List all HID devices connected to my computer.

> Open the HID device at /dev/hidraw0 and write hex data 00010203 to it.

## Example: Running as SSE Server

```bash
# Start SSE server on port 8002
simplecommkitaihid-fastmcpp --transport sse --port 8002

# In another terminal, test with curl:

# List devices
curl -X POST "http://127.0.0.1:8002/messages" \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_available_devices","arguments":{}}}'
```

## Example: C++ Client Integration

```cpp
#include <fastmcpp/client/client.hpp>
#include <fastmcpp/client/transports.hpp>

// Connect to the MCP server via stdio
fastmcpp::client::StdioTransport transport("simplecommkitaihid-fastmcpp");
fastmcpp::client::Client client(transport);

// Initialize
client.initialize();

// Call get_available_devices
auto result = client.call_tool("get_available_devices", {});
// result.content contains the device list
```

## Example: Hotplug Monitoring via MCP

1. Start hotplug:
   ```
   start_hotplug(vendor_id=0, product_id=0, poll_interval_ms=2000)
   ```

2. Periodically check the device list:
   ```
   get_device_list()
   ```

3. Stop when done:
   ```
   stop_hotplug()
   ```

## Example: Read Data Loop

1. Open a device in readable mode:
   ```
   open_device(path="/dev/hidraw0", readable=true)
   ```

2. Periodically drain buffered data:
   ```
   get_read_data(path="/dev/hidraw0")
   ```

3. Each call returns accumulated data since the last call, then clears the buffer.

## Example: Configure Fast Polling

For devices that send data at high rates, reduce the poll interval:

```
set_read_config(path="/dev/hidraw0", poll_interval_ms=10, data_length=128)
```

This polls every 10ms and reads up to 128 bytes per poll.
