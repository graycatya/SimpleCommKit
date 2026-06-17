#include "ble_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace SimpleCommKitAiBleFastmcpp
{

// ===========================================================================
// Schema builders
// ===========================================================================

static fastmcpp::Json prop_string(const char* desc, const char* default_val = nullptr)
{
    fastmcpp::Json p;
    p["type"] = "string";
    p["description"] = desc;
    if (default_val)
        p["default"] = default_val;
    return p;
}

static fastmcpp::Json prop_integer(const char* desc, int default_val = 0)
{
    fastmcpp::Json p;
    p["type"] = "integer";
    p["description"] = desc;
    p["default"] = default_val;
    return p;
}

static fastmcpp::Json prop_boolean(const char* desc, bool default_val)
{
    fastmcpp::Json p;
    p["type"] = "boolean";
    p["description"] = desc;
    p["default"] = default_val;
    return p;
}

static fastmcpp::Json make_object_schema(
    std::initializer_list<std::pair<const char*, fastmcpp::Json>> props,
    std::initializer_list<const char*> required = {})
{
    fastmcpp::Json schema;
    schema["type"] = "object";
    for (auto& kv : props)
        schema["properties"][kv.first] = kv.second;
    if (required.size() > 0)
        schema["required"] = required;
    return schema;
}

// ===========================================================================
// Helper: select the current peripheral by address
// ===========================================================================

static SimpleCommKit::SimpleCommKitBlePeripheral&
select_peripheral(const std::string& address)
{
    auto& bs = BleState::instance();
    std::lock_guard<std::mutex> lock(bs.peripherals_mutex_);

    auto it = bs.peripherals_.find(address);
    if (it == bs.peripherals_.end())
    {
        // Try to find from scan results
        std::lock_guard<std::mutex> slock(bs.scan_results_mutex_);
        for (auto& p : bs.scan_results)
        {
            if (p.address == address)
            {
                bs.peripherals_[address].peripheral = p;
                break;
            }
        }
    }

    it = bs.peripherals_.find(address);
    if (it == bs.peripherals_.end())
        throw std::runtime_error("Peripheral not found: " + address +
                                 ". Run scan_for first.");

    bs.ensure_ble().set_CurrentPeripheral(it->second.peripheral);
    return it->second.peripheral;
}

// ===========================================================================
// Helper: resolve peripheral for an address, returning a cached copy
// ===========================================================================

static SimpleCommKit::SimpleCommKitBlePeripheral
resolve_peripheral(const std::string& address)
{
    auto& bs = BleState::instance();
    {
        std::lock_guard<std::mutex> lock(bs.peripherals_mutex_);
        auto it = bs.peripherals_.find(address);
        if (it != bs.peripherals_.end())
            return it->second.peripheral;
    }
    {
        std::lock_guard<std::mutex> lock(bs.scan_results_mutex_);
        for (auto& p : bs.scan_results)
        {
            if (p.address == address)
            {
                bs.peripherals_[address].peripheral = p;
                return p;
            }
        }
    }
    throw std::runtime_error("Peripheral not found: " + address + ". Run scan_for first.");
}

// ===========================================================================
// Helper: content array wrapper for structured JSON results
// ===========================================================================

static fastmcpp::Json make_content(const fastmcpp::Json& data)
{
    return fastmcpp::Json{
        {"content",
         fastmcpp::Json::array({fastmcpp::Json{{"type", "text"}, {"text", data.dump(2)}}})}};
}

// ===========================================================================
// Register all 16 BLE MCP tools
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. bluetooth_enabled
    // -----------------------------------------------------------------------
    app.tool(
        "bluetooth_enabled",
        [](const Json&) -> Json
        {
            bool enabled = SimpleCommKit::SimpleCommKitBleCentral::Bluetooth_Enabled();
            return Json{{"enabled", enabled},
                        {"message", enabled ? "Bluetooth is enabled" : "Bluetooth is disabled"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Check if Bluetooth is enabled on this system."});

    // -----------------------------------------------------------------------
    // 2. get_adapters
    // -----------------------------------------------------------------------
    app.tool(
        "get_adapters",
        [](const Json&) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();
            auto adapters = ble.get_Adapters();

            if (!adapters.empty())
            {
                bs.current_adapter_ = adapters[0];
                ble.set_CurrentAdapter(adapters[0]);
            }

            Json arr = Json::array();
            for (const auto& a : adapters)
                arr.push_back(adapter_to_json(a));
            return make_content(arr);
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "List available Bluetooth adapters on this system."});

    // -----------------------------------------------------------------------
    // 3. scan_for
    // -----------------------------------------------------------------------
    app.tool(
        "scan_for",
        make_object_schema({
            {"timeout_ms", prop_integer("Scan timeout in milliseconds (default 5000)", 5000)},
        }),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            int timeout_ms = input.value("timeout_ms", 5000);

            // Ensure an adapter is selected
            if (!bs.current_adapter_.has_value())
            {
                auto adapters = ble.get_Adapters();
                if (adapters.empty())
                    throw std::runtime_error("No Bluetooth adapter found");
                bs.current_adapter_ = adapters[0];
                ble.set_CurrentAdapter(adapters[0]);
            }

            // Set scan callbacks (lazy init, capture nothing, use singleton)
            ble.adapter_Set_Callback_On_Scan_Found(
                [](SimpleCommKit::SimpleCommKitBlePeripheral p) {
                    BleState::instance().add_scan_result(p);
                });
            ble.adapter_Set_Callback_On_Scan_Updated(
                [](SimpleCommKit::SimpleCommKitBlePeripheral p) {
                    BleState::instance().add_scan_result(p);
                });

            bs.clear_scan_results();

            // Ensure adapter is on
            if (!ble.adapter_Is_Powered())
                ble.adapter_Power_On();

            // Scan for the specified duration (blocking)
            ble.adapter_Scan_For(timeout_ms);

            // Collect results
            std::vector<SimpleCommKit::SimpleCommKitBlePeripheral> results;
            {
                std::lock_guard<std::mutex> lock(bs.scan_results_mutex_);
                results = bs.scan_results;
            }

            // Cache results in peripherals_ for quick lookup
            {
                std::lock_guard<std::mutex> lock(bs.peripherals_mutex_);
                for (const auto& p : results)
                {
                    if (bs.peripherals_.find(p.address) == bs.peripherals_.end())
                        bs.peripherals_[p.address].peripheral = p;
                    else
                        bs.peripherals_[p.address].peripheral = p; // update
                }
            }

            Json arr = Json::array();
            for (const auto& p : results)
                arr.push_back(peripheral_to_json(p));

            return Json{
                {"content",
                 Json::array({Json{{"type", "text"},
                                   {"text",
                                    "Found " + std::to_string(results.size()) +
                                        " device(s).\n\n" + arr.dump(2)}}})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Scan for nearby Bluetooth peripherals. "
            "Returns a list of discovered devices with address, RSSI, and manufacturer data."});

    // -----------------------------------------------------------------------
    // 4. connect
    // -----------------------------------------------------------------------
    app.tool(
        "connect",
        make_object_schema({
            {"address", prop_string("Address of the peripheral to connect to")},
        }, {"address"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");

            // Find the peripheral from cache
            auto peripheral = resolve_peripheral(address);

            // Set as current peripheral
            ble.set_CurrentPeripheral(peripheral);

            // Connect
            ble.peripheral_Connect();

            // Mark as connected
            bs.set_connected(address, true);

            return Json{{"message", "Connected to " + address}, {"address", address}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Connect to a Bluetooth peripheral. "
            "The device must have been discovered via scan_for first."});

    // -----------------------------------------------------------------------
    // 5. disconnect
    // -----------------------------------------------------------------------
    app.tool(
        "disconnect",
        make_object_schema({
            {"address", prop_string("Address of the peripheral to disconnect")},
        }, {"address"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");

            select_peripheral(address);
            ble.peripheral_Disconnect();

            bs.set_connected(address, false);
            bs.remove_notification_buffer(address);

            return Json{{"message", "Disconnected from " + address}, {"address", address}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Disconnect from a Bluetooth peripheral. "
            "Also clears any buffered notifications for this device."});

    // -----------------------------------------------------------------------
    // 6. connected_devices
    // -----------------------------------------------------------------------
    app.tool(
        "connected_devices",
        [](const Json&) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            auto connected = ble.adapter_Get_Connected_Peripherals();

            Json arr = Json::array();
            for (const auto& p : connected)
                arr.push_back(peripheral_to_json(p));
            return make_content(arr);
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "List currently connected Bluetooth peripherals."});

    // -----------------------------------------------------------------------
    // 7. services
    // -----------------------------------------------------------------------
    app.tool(
        "services",
        make_object_schema({
            {"address", prop_string("Address of the connected peripheral")},
        }, {"address"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");
            select_peripheral(address);

            auto svcs = ble.peripheral_Services();

            Json arr = Json::array();
            for (const auto& s : svcs)
                arr.push_back(service_to_json(s));
            return make_content(arr);
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "List GATT services and characteristics for a connected peripheral."});

    // -----------------------------------------------------------------------
    // 8. read
    // -----------------------------------------------------------------------
    app.tool(
        "read",
        make_object_schema({
            {"address",           prop_string("Address of the connected peripheral")},
            {"service_uuid",      prop_string("Service UUID (e.g. '0000180a-0000-1000-8000-00805f9b34fb')")},
            {"characteristic_uuid", prop_string("Characteristic UUID")},
        }, {"address", "service_uuid", "characteristic_uuid"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");
            std::string svc = input.value("service_uuid", "");
            std::string ch = input.value("characteristic_uuid", "");

            select_peripheral(address);

            auto data = ble.peripheral_Read(svc, ch);

            return Json{{"address", address},
                        {"service_uuid", svc},
                        {"characteristic_uuid", ch},
                        {"data_hex", bytes_to_hex(data)},
                        {"data_utf8", bytes_to_utf8_safe(data)},
                        {"data_length", data.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Read the value of a GATT characteristic from a connected peripheral."});

    // -----------------------------------------------------------------------
    // 9. write_request
    // -----------------------------------------------------------------------
    app.tool(
        "write_request",
        make_object_schema({
            {"address",            prop_string("Address of the connected peripheral")},
            {"service_uuid",       prop_string("Service UUID")},
            {"characteristic_uuid", prop_string("Characteristic UUID")},
            {"data",               prop_string("Hex string data to write (e.g. '00FF' or '01 02 AA')")},
        }, {"address", "service_uuid", "characteristic_uuid", "data"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");
            std::string svc = input.value("service_uuid", "");
            std::string ch = input.value("characteristic_uuid", "");
            std::string data_hex = input.value("data", "");

            select_peripheral(address);

            auto data_bytes = hex_to_bytes(data_hex);
            ble.peripheral_Write_Request(svc, ch, data_bytes);

            return Json{{"message", "Write request sent"},
                        {"address", address},
                        {"bytes_written", data_bytes.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Write data to a GATT characteristic with response (write request). "
            "Data must be a hex string (e.g. '00FF' or '01 02 AA')."});

    // -----------------------------------------------------------------------
    // 10. write_command
    // -----------------------------------------------------------------------
    app.tool(
        "write_command",
        make_object_schema({
            {"address",            prop_string("Address of the connected peripheral")},
            {"service_uuid",       prop_string("Service UUID")},
            {"characteristic_uuid", prop_string("Characteristic UUID")},
            {"data",               prop_string("Hex string data to write (e.g. '00FF')")},
        }, {"address", "service_uuid", "characteristic_uuid", "data"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");
            std::string svc = input.value("service_uuid", "");
            std::string ch = input.value("characteristic_uuid", "");
            std::string data_hex = input.value("data", "");

            select_peripheral(address);

            auto data_bytes = hex_to_bytes(data_hex);
            ble.peripheral_Write_Command(svc, ch, data_bytes);

            return Json{{"message", "Write command sent"},
                        {"address", address},
                        {"bytes_written", data_bytes.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Write data to a GATT characteristic without response (write command). "
            "Faster than write_request but does not confirm delivery. "
            "Data must be a hex string."});

    // -----------------------------------------------------------------------
    // 11. descriptor_read
    // -----------------------------------------------------------------------
    app.tool(
        "descriptor_read",
        make_object_schema({
            {"address",            prop_string("Address of the connected peripheral")},
            {"service_uuid",       prop_string("Service UUID")},
            {"characteristic_uuid", prop_string("Characteristic UUID")},
            {"descriptor_uuid",    prop_string("Descriptor UUID")},
        }, {"address", "service_uuid", "characteristic_uuid", "descriptor_uuid"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");
            std::string svc = input.value("service_uuid", "");
            std::string ch = input.value("characteristic_uuid", "");
            std::string desc = input.value("descriptor_uuid", "");

            select_peripheral(address);

            auto data = ble.peripheral_Read(svc, ch, desc);

            return Json{{"address", address},
                        {"service_uuid", svc},
                        {"characteristic_uuid", ch},
                        {"descriptor_uuid", desc},
                        {"data_hex", bytes_to_hex(data)},
                        {"data_utf8", bytes_to_utf8_safe(data)},
                        {"data_length", data.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Read the value of a GATT descriptor from a connected peripheral."});

    // -----------------------------------------------------------------------
    // 12. descriptor_write
    // -----------------------------------------------------------------------
    app.tool(
        "descriptor_write",
        make_object_schema({
            {"address",            prop_string("Address of the connected peripheral")},
            {"service_uuid",       prop_string("Service UUID")},
            {"characteristic_uuid", prop_string("Characteristic UUID")},
            {"descriptor_uuid",    prop_string("Descriptor UUID")},
            {"data",               prop_string("Hex string data to write (e.g. '0100')")},
        }, {"address", "service_uuid", "characteristic_uuid", "descriptor_uuid", "data"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");
            std::string svc = input.value("service_uuid", "");
            std::string ch = input.value("characteristic_uuid", "");
            std::string desc = input.value("descriptor_uuid", "");
            std::string data_hex = input.value("data", "");

            select_peripheral(address);

            auto data_bytes = hex_to_bytes(data_hex);
            ble.peripheral_Write(svc, ch, desc, data_bytes);

            return Json{{"message", "Descriptor write sent"},
                        {"address", address},
                        {"bytes_written", data_bytes.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Write data to a GATT descriptor. Data must be a hex string."});

    // -----------------------------------------------------------------------
    // 13. notify
    // -----------------------------------------------------------------------
    app.tool(
        "notify",
        make_object_schema({
            {"address",            prop_string("Address of the connected peripheral")},
            {"service_uuid",       prop_string("Service UUID")},
            {"characteristic_uuid", prop_string("Characteristic UUID")},
        }, {"address", "service_uuid", "characteristic_uuid"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");
            std::string svc = input.value("service_uuid", "");
            std::string ch = input.value("characteristic_uuid", "");

            if (bs.has_subscription(address, svc, ch))
            {
                return Json{{"message", "Already subscribed to notifications on " + ch},
                            {"address", address},
                            {"characteristic_uuid", ch}};
            }

            select_peripheral(address);

            // Setup notification callback that routes data into our buffer
            ble.peripheral_Notify(svc, ch,
                [address, svc, ch](std::vector<uint8_t> payload) {
                    BleState::instance().push_notification(address, svc, ch, payload);
                });

            bs.add_subscription(address, svc, ch);

            return Json{{"message", "Subscribed to notifications"},
                        {"address", address},
                        {"service_uuid", svc},
                        {"characteristic_uuid", ch}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Subscribe to notifications on a GATT characteristic. "
            "Use get_notifications to retrieve buffered data."});

    // -----------------------------------------------------------------------
    // 14. indicate
    // -----------------------------------------------------------------------
    app.tool(
        "indicate",
        make_object_schema({
            {"address",            prop_string("Address of the connected peripheral")},
            {"service_uuid",       prop_string("Service UUID")},
            {"characteristic_uuid", prop_string("Characteristic UUID")},
        }, {"address", "service_uuid", "characteristic_uuid"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");
            std::string svc = input.value("service_uuid", "");
            std::string ch = input.value("characteristic_uuid", "");

            if (bs.has_subscription(address, svc, ch))
            {
                return Json{{"message", "Already subscribed to indications on " + ch},
                            {"address", address},
                            {"characteristic_uuid", ch}};
            }

            select_peripheral(address);

            ble.peripheral_Indicate(svc, ch,
                [address, svc, ch](std::vector<uint8_t> payload) {
                    BleState::instance().push_notification(address, svc, ch, payload);
                });

            bs.add_subscription(address, svc, ch);

            return Json{{"message", "Subscribed to indications"},
                        {"address", address},
                        {"service_uuid", svc},
                        {"characteristic_uuid", ch}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Subscribe to indications on a GATT characteristic (acknowledged notifications). "
            "Use get_notifications to retrieve buffered data."});

    // -----------------------------------------------------------------------
    // 15. get_notifications
    // -----------------------------------------------------------------------
    app.tool(
        "get_notifications",
        make_object_schema({
            {"address", prop_string("Address of the peripheral to retrieve notifications for")},
        }, {"address"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            std::string address = input.value("address", "");

            auto entries = bs.drain_notifications(address);

            Json arr = Json::array();
            for (const auto& e : entries)
            {
                arr.push_back(Json{{"address", e.address},
                                   {"service_uuid", e.service_uuid},
                                   {"characteristic_uuid", e.characteristic_uuid},
                                   {"data_hex", e.data_hex},
                                   {"data_utf8", e.data_utf8},
                                   {"data_length", e.data_length}});
            }
            return make_content(arr);
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Retrieve and clear buffered notification/indication data for a device. "
            "Data is buffered after calling notify or indicate."});

    // -----------------------------------------------------------------------
    // 16. unsubscribe
    // -----------------------------------------------------------------------
    app.tool(
        "unsubscribe",
        make_object_schema({
            {"address",            prop_string("Address of the connected peripheral")},
            {"service_uuid",       prop_string("Service UUID")},
            {"characteristic_uuid", prop_string("Characteristic UUID")},
        }, {"address", "service_uuid", "characteristic_uuid"}),
        [](const Json& input) -> Json
        {
            auto& bs = BleState::instance();
            auto& ble = bs.ensure_ble();

            std::string address = input.value("address", "");
            std::string svc = input.value("service_uuid", "");
            std::string ch = input.value("characteristic_uuid", "");

            select_peripheral(address);

            ble.peripheral_Unsubscribe(svc, ch);
            bs.remove_subscription(address, svc, ch);

            return Json{{"message", "Unsubscribed from " + ch},
                        {"address", address},
                        {"characteristic_uuid", ch}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Unsubscribe from notifications/indications on a GATT characteristic."});
}

} // namespace SimpleCommKitAiBleFastmcpp
