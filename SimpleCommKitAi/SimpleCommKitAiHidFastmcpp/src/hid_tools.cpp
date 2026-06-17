#include "hid_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace SimpleCommKitAiHidFastmcpp
{

// ===========================================================================
// Schema builders — use nlohmann::json operator[] instead of initializer lists
// or raw string literals, both of which MSVC (locale 936) struggles with.
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

static fastmcpp::Json make_object_schema(std::initializer_list<std::pair<const char*, fastmcpp::Json>> props,
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
// Register all 11 HID MCP tools on a FastMCP application instance.
// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiHid.
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. get_available_devices
    // -----------------------------------------------------------------------
    app.tool(
        "get_available_devices",
        make_object_schema({
            {"vendor_id",  prop_integer("USB vendor ID filter (0 = all vendors)")},
            {"product_id", prop_integer("USB product ID filter (0 = all products)")},
        }),
        [](const Json& input) -> Json
        {
            auto& hs = HidState::instance();
            hs.ensure_hid();

            unsigned short vid = static_cast<unsigned short>(input.value("vendor_id", 0));
            unsigned short pid = static_cast<unsigned short>(input.value("product_id", 0));

            auto devices = SimpleCommKit::SimpleCommKitHid::get_Available_Devices(vid, pid);
            hs.device_cache = devices;

            Json arr = Json::array();
            for (const auto& d : devices)
                arr.push_back(device_info_to_json(d));
            return Json{{"content",
                         Json::array({Json{{"type", "text"}, {"text", arr.dump(2)}}})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "List available HID (Human Interface Device) devices on the host. "
            "Optionally filter by vendor_id and product_id."});

    // -----------------------------------------------------------------------
    // 2. open_device
    // -----------------------------------------------------------------------
    app.tool(
        "open_device",
        make_object_schema({
            {"path",          prop_string ("Platform-specific device path (takes priority)", "")},
            {"vendor_id",     prop_integer("USB vendor ID (used when path is empty)")},
            {"product_id",    prop_integer("USB product ID (used when path is empty)")},
            {"serial_number", prop_string ("Optional serial number filter", "")},
            {"readable",      prop_boolean("Whether to enable read mode (default true)", true)},
        }),
        [](const Json& input) -> Json
        {
            auto& hs = HidState::instance();
            auto& hid = hs.ensure_hid();

            std::string path = input.value("path", "");
            unsigned short vid = static_cast<unsigned short>(input.value("vendor_id", 0));
            unsigned short pid = static_cast<unsigned short>(input.value("product_id", 0));
            std::string serial = input.value("serial_number", "");
            bool readable = input.value("readable", true);

            if (!hid.init())
                throw std::runtime_error("HID init failed");

            std::string opened_path;

            if (!path.empty())
            {
                if (hid.is_Open(path))
                    return Json{{"message", "Device already open: " + path}, {"path", path}};

                if (readable)
                    hs.setup_read_callback();

                if (!hid.open(path, readable))
                    throw std::runtime_error("Failed to open " + path);

                opened_path = path;
            }
            else if (vid != 0 && pid != 0)
            {
                if (readable)
                    hs.setup_read_callback();

                if (!hid.open(vid, pid, serial, readable))
                    throw std::runtime_error("No HID device matched VID/PID");

                // Resolve actual device path: the library returns internal keys
                // (e.g. "2362:9523:*") but read data is keyed by OS device path.
                // Enumerate to find the real Windows path for this device.
                auto devices = SimpleCommKit::SimpleCommKitHid::get_Available_Devices(vid, pid);
                opened_path = devices.empty() ? "" : devices.front().path;
            }
            else
            {
                throw std::runtime_error(
                    "Must provide either 'path' or both 'vendor_id' and 'product_id'");
            }

            return Json{{"message", "Opened device"}, {"path", opened_path}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Open a HID device for communication. You must provide either a "
            "path or a combination of vendor_id + product_id + optional "
            "serial_number. Set readable=false for write-only mode."});

    // -----------------------------------------------------------------------
    // 3. close_device
    // -----------------------------------------------------------------------
    app.tool(
        "close_device",
        make_object_schema({
            {"path", prop_string("Device path to close (omitted = close all)", "")},
        }),
        [](const Json& input) -> Json
        {
            auto& hs = HidState::instance();
            auto& hid = hs.ensure_hid();

            std::string path = input.value("path", "");

            if (!path.empty())
            {
                hid.close(path);
                hs.remove_read_buffer(path);
                return Json{{"message", "Closed device: " + path}};
            }
            else
            {
                hid.close();
                hs.clear_read_buffers();
                return Json{{"message", "Closed all devices"}};
            }
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Close a HID device. Provide a path to close a specific device, "
            "or omit to close all open devices."});

    // -----------------------------------------------------------------------
    // 4. write
    // -----------------------------------------------------------------------
    app.tool(
        "write",
        make_object_schema({
            {"path", prop_string("Path of the open HID device")},
            {"data", prop_string("Hex string to write (e.g. '00FF' or '01 02 AA')")},
        }, {"path", "data"}),
        [](const Json& input) -> Json
        {
            auto& hs = HidState::instance();
            auto& hid = hs.ensure_hid();

            std::string path = input.value("path", "");
            std::string data_hex = input.value("data", "");

            if (!hid.is_Open(path))
                throw std::runtime_error("Device not open: " + path);

            auto data_bytes = hex_to_bytes(data_hex);
            int written = hid.write(path, data_bytes);

            if (written < 0)
                throw std::runtime_error("Write failed");

            return Json{{"message", "Wrote " + std::to_string(written) + " byte(s)"},
                        {"path", path},
                        {"bytes_written", written}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Write a report to a HID device. Data must be a hex string "
            "(e.g. '00FF' or '01 02 AA')."});

    // -----------------------------------------------------------------------
    // 5. send_feature_report
    // -----------------------------------------------------------------------
    app.tool(
        "send_feature_report",
        make_object_schema({
            {"path", prop_string("Path of the open HID device")},
            {"data", prop_string("Hex string (e.g. '00FF' or '01 02 AA')")},
        }, {"path", "data"}),
        [](const Json& input) -> Json
        {
            auto& hs = HidState::instance();
            auto& hid = hs.ensure_hid();

            std::string path = input.value("path", "");
            std::string data_hex = input.value("data", "");

            if (!hid.is_Open(path))
                throw std::runtime_error("Device not open: " + path);

            auto data_bytes = hex_to_bytes(data_hex);
            int sent = hid.send_Feature_Report(path, data_bytes);

            if (sent < 0)
                throw std::runtime_error("Feature report failed");

            return Json{{"message", "Feature report sent (" + std::to_string(sent) + " byte(s))"},
                        {"path", path}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Send a HID feature report to a device. Data must be a hex string."});

    // -----------------------------------------------------------------------
    // 6. start_hotplug
    // -----------------------------------------------------------------------
    app.tool(
        "start_hotplug",
        make_object_schema({
            {"vendor_id",        prop_integer("USB vendor ID filter (0 = all vendors)")},
            {"product_id",       prop_integer("USB product ID filter (0 = all products)")},
            {"poll_interval_ms", prop_integer("Poll interval in milliseconds (default 1000)", 1000)},
        }),
        [](const Json& input) -> Json
        {
            auto& hs = HidState::instance();
            auto& hid = hs.ensure_hid();

            unsigned short vid = static_cast<unsigned short>(input.value("vendor_id", 0));
            unsigned short pid = static_cast<unsigned short>(input.value("product_id", 0));
            int poll_ms = input.value("poll_interval_ms", 1000);

            if (!hid.init(vid, pid))
                throw std::runtime_error("HID init failed");

            if (poll_ms > 0)
                hid.set_Hotplug_Poll_Interval(poll_ms);

            hid.set_Callback_On_HotPlug(
                [](const std::vector<SimpleCommKit::SimpleCommKitHidDeviceInfo>& added,
                   const std::vector<SimpleCommKit::SimpleCommKitHidDeviceInfo>& removed)
                {
                    std::cerr << "[SimpleCommKitAiHidFastmcpp] Hotplug: +"
                              << added.size() << " -" << removed.size() << std::endl;
                });

            hid.start_Hotplug(vid, pid);
            hs.hotplug_active = true;

            return Json{{"message", "Hotplug detection started"},
                        {"vendor_id", vid},
                        {"product_id", pid},
                        {"poll_interval_ms", hid.get_Hotplug_Poll_Interval()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Start HID device hotplug detection (polling-based). "
            "Provide vendor_id and product_id to filter, or 0 for all devices."});

    // -----------------------------------------------------------------------
    // 7. stop_hotplug
    // -----------------------------------------------------------------------
    app.tool(
        "stop_hotplug",
        [](const Json&) -> Json
        {
            auto& hs = HidState::instance();
            auto& hid = hs.ensure_hid();

            hid.stop_Hotplug();
            hs.hotplug_active = false;

            return Json{{"message", "Hotplug detection stopped"}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Stop HID device hotplug detection."});

    // -----------------------------------------------------------------------
    // 8. get_device_list
    // -----------------------------------------------------------------------
    app.tool(
        "get_device_list",
        [](const Json&) -> Json
        {
            auto& hs = HidState::instance();
            auto& hid = hs.ensure_hid();

            auto devices = hid.get_Device_List();
            hs.device_cache = devices;

            Json arr = Json::array();
            for (const auto& d : devices)
                arr.push_back(device_info_to_json(d));
            return Json{{"content",
                         Json::array({Json{{"type", "text"}, {"text", arr.dump(2)}}})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Get the cached list of devices (populated by open_device or start_hotplug)."});

    // -----------------------------------------------------------------------
    // 9. get_open_paths
    // -----------------------------------------------------------------------
    app.tool(
        "get_open_paths",
        [](const Json&) -> Json
        {
            auto& hs = HidState::instance();
            auto& hid = hs.ensure_hid();

            auto paths = hid.get_Open_Paths();
            bool hotplug_active = hid.is_Hotplug_Active();

            return Json{{"open_paths", paths},
                        {"open_count", paths.size()},
                        {"hotplug_active", hotplug_active}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Get a list of currently open HID device paths and their status."});

    // -----------------------------------------------------------------------
    // 10. get_read_data
    // -----------------------------------------------------------------------
    app.tool(
        "get_read_data",
        make_object_schema({
            {"path", prop_string("Path of the open HID device to read from")},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            auto& hs = HidState::instance();
            std::string path = input.value("path", "");

            auto entries = hs.drain_read_buffer(path);

            Json arr = Json::array();
            for (const auto& e : entries)
            {
                arr.push_back(Json{{"path", e.path},
                                   {"data_hex", e.data_hex},
                                   {"data_utf8", e.data_utf8},
                                   {"data_length", e.data_length}});
            }
            return Json{{"content",
                         Json::array({Json{{"type", "text"}, {"text", arr.dump(2)}}})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Retrieve and clear buffered read data for a device. "
            "Data is buffered after calling open_device with readable=true."});

    // -----------------------------------------------------------------------
    // 11. set_read_config
    // -----------------------------------------------------------------------
    app.tool(
        "set_read_config",
        make_object_schema({
            {"path",              prop_string ("Path of the open HID device to configure")},
            {"poll_interval_ms",  prop_integer("How often to poll for data in ms (0 = no change)")},
            {"data_length",       prop_integer("Bytes to read per poll (0 = no change, default 64)")},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            auto& hs = HidState::instance();
            auto& hid = hs.ensure_hid();

            std::string path = input.value("path", "");
            int poll_ms = input.value("poll_interval_ms", 0);
            int data_len = input.value("data_length", 0);

            if (poll_ms > 0)
                hid.set_Read_Poll_Interval(path, poll_ms);
            if (data_len > 0)
                hid.set_Read_Data_Length(path, data_len);

            return Json{
                {"message", "Read config updated"},
                {"path", path},
                {"read_poll_interval_ms", hid.get_Read_Poll_Interval(path)},
                {"read_data_length", hid.get_Read_Data_Length(path)}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Configure read polling parameters for a HID device. "
            "Set poll_interval_ms (how often to poll, in ms) and/or "
            "data_length (bytes to read per poll, default 64)."});
}

} // namespace SimpleCommKitAiHidFastmcpp
