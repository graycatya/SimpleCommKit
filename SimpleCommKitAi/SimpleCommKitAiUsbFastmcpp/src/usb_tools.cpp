#include "usb_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace SimpleCommKitAiUsbFastmcpp
{

// ===========================================================================
// Schema builders — use nlohmann::json operator[] for MSVC compatibility
// ===========================================================================

static fastmcpp::Json prop_string(const char* desc, const char* default_val = nullptr)
{
    fastmcpp::Json p;
    p["type"]        = "string";
    p["description"] = desc;
    if (default_val)
        p["default"] = default_val;
    return p;
}

static fastmcpp::Json prop_integer(const char* desc, int default_val = 0)
{
    fastmcpp::Json p;
    p["type"]        = "integer";
    p["description"] = desc;
    p["default"]     = default_val;
    return p;
}

static fastmcpp::Json prop_boolean(const char* desc, bool default_val)
{
    fastmcpp::Json p;
    p["type"]        = "boolean";
    p["description"] = desc;
    p["default"]     = default_val;
    return p;
}

static fastmcpp::Json prop_number(const char* desc, double default_val = 0.0)
{
    fastmcpp::Json p;
    p["type"]        = "number";
    p["description"] = desc;
    p["default"]     = default_val;
    return p;
}

static fastmcpp::Json prop_array(const char* desc)
{
    fastmcpp::Json p;
    p["type"]        = "array";
    p["description"] = desc;
    // items are integers
    fastmcpp::Json items;
    items["type"] = "integer";
    p["items"]    = items;
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
// Helper: resolve a device by path (throws if not found / not open)
// ===========================================================================

static SimpleCommKit::SimpleCommKitUsb* resolve_device(const std::string& path,
                                                        bool check_open = true)
{
    auto& us = UsbState::instance();
    auto* dev = us.get_device(path);
    if (!dev)
        throw std::runtime_error("Device not found: " + path + ". Open it first with 'open'.");
    if (check_open && !dev->is_Open())
        throw std::runtime_error("Device not open: " + path + ". Open it first with 'open'.");
    return dev;
}

// ===========================================================================
// Helper: content array wrapper for structured JSON results
// ===========================================================================

static fastmcpp::Json make_content(const fastmcpp::Json& data)
{
    fastmcpp::Json text_entry;
    text_entry["type"] = "text";
    text_entry["text"] = data.dump(2);

    return fastmcpp::Json{{"content", fastmcpp::Json::array({text_entry})}};
}

// ===========================================================================
// Helper: build a device-status JSON response
// ===========================================================================

static fastmcpp::Json build_device_status_json(SimpleCommKit::SimpleCommKitUsb& dev,
                                                const std::string& path)
{
    fastmcpp::Json status;
    status["path"]            = path;
    status["is_open"]         = dev.is_Open();
    status["is_read_polling"] = dev.is_Read_Poll_Active();
    status["is_hotplug_active"] = dev.is_Hotplug_Active();
    if (dev.is_Open())
    {
        status["open_path"]          = dev.get_Open_Path();
        status["read_poll_interval"] = dev.get_Read_Poll_Interval();
        status["read_data_length"]   = dev.get_Read_Data_Length();
        status["hotplug_poll_interval"] = dev.get_Hotplug_Poll_Interval();
    }
    return status;
}

// ===========================================================================
// Register all 23 USB MCP tools
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
            {"vendor_id",  prop_integer("Filter by vendor ID (0 = no filter)", 0)},
            {"product_id", prop_integer("Filter by product ID (0 = no filter)", 0)},
        }),
        [](const Json& input) -> Json
        {
            unsigned short vid = static_cast<unsigned short>(
                input.value("vendor_id", 0));
            unsigned short pid = static_cast<unsigned short>(
                input.value("product_id", 0));

            auto devices =
                SimpleCommKit::SimpleCommKitUsb::get_Available_Devices(vid, pid);

            Json arr = Json::array();
            for (const auto& d : devices)
                arr.push_back(device_info_to_json(d));

            Json text_entry;
            text_entry["type"] = "text";
            text_entry["text"] = "Found " + std::to_string(devices.size()) +
                                 " USB device(s).\n\n" + arr.dump(2);

            return Json{{"content", Json::array({text_entry})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Enumerate available USB devices. "
            "Optionally filter by vendor_id and/or product_id. "
            "Returns device info including path, VID, PID, and serial number."});

    // -----------------------------------------------------------------------
    // 2. open
    // -----------------------------------------------------------------------
    app.tool(
        "open",
        make_object_schema({
            {"path",          prop_string("Device path (e.g. '1:3'). Use this OR (vendor_id + product_id).", "")},
            {"vendor_id",     prop_integer("Vendor ID (hex e.g. 0x1234 -> 4660). Used with product_id.", 0)},
            {"product_id",    prop_integer("Product ID. Used with vendor_id.", 0)},
            {"serial_number", prop_string("Optional serial number filter when using VID/PID", "")},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            auto& us = UsbState::instance();

            std::string path = input.value("path", "");
            unsigned short vid = static_cast<unsigned short>(
                input.value("vendor_id", 0));
            unsigned short pid = static_cast<unsigned short>(
                input.value("product_id", 0));
            std::string serial = input.value("serial_number", "");

            // Determine the device key (path or vid:pid:serial)
            std::string key;
            bool        use_vidpid = false;

            if (!path.empty())
            {
                key = path;
            }
            else if (vid != 0 || pid != 0)
            {
                use_vidpid = true;
                key = std::to_string(vid) + ":" + std::to_string(pid);
                if (!serial.empty())
                    key += ":" + serial;
            }
            else
            {
                throw std::runtime_error(
                    "Either 'path' or ('vendor_id' + 'product_id') must be provided.");
            }

            // Check if already open
            if (us.has_device(key))
            {
                auto* existing = us.get_device(key);
                if (existing && existing->is_Open())
                    return Json{{"message", "Device already open: " + key},
                                {"path", key}};
            }

            auto* dev = us.get_or_create_device(key);

            // Initialize libusb
            if (!dev->init())
                throw std::runtime_error("Failed to initialize USB subsystem for " + key);

            // Open the device
            bool ok = false;
            if (use_vidpid)
            {
                ok = dev->open(vid, pid, serial);
            }
            else
            {
                ok = dev->open(path);
            }

            if (!ok)
                throw std::runtime_error("Failed to open USB device: " + key);

            // Install read and hotplug callbacks
            us.setup_read_callback(key);
            us.setup_hotplug_callback(key);

            return Json{{"message", "Opened USB device: " + key},
                        {"path", key}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Open a USB device. Provide either a 'path' (e.g. '1:3' from get_available_devices), "
            "or 'vendor_id' + 'product_id' (+ optional 'serial_number'). "
            "The device will be ready for interface claiming and transfers."});

    // -----------------------------------------------------------------------
    // 3. close
    // -----------------------------------------------------------------------
    app.tool(
        "close",
        make_object_schema({
            {"path", prop_string("Device path to close (omitted = close all)", "")},
        }),
        [](const Json& input) -> Json
        {
            auto& us = UsbState::instance();
            std::string path = input.value("path", "");

            if (!path.empty())
            {
                auto* dev = us.get_device(path);
                if (dev)
                {
                    if (dev->is_Read_Poll_Active())
                        dev->stop_Read_Poll();
                    if (dev->is_Hotplug_Active())
                        dev->stop_Hotplug();
                    if (dev->is_Open())
                        dev->close();
                }
                us.remove_device(path);
                return Json{{"message", "Closed USB device: " + path}};
            }
            else
            {
                // Close all
                us.remove_all_devices();
                return Json{{"message", "Closed all USB devices"}};
            }
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Close a USB device. Provide a path to close a specific device, "
            "or omit to close all open devices. Also stops read-poll and hotplug if active."});

    // -----------------------------------------------------------------------
    // 4. is_open
    // -----------------------------------------------------------------------
    app.tool(
        "is_open",
        make_object_schema({
            {"path", prop_string("Device path to check")},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            auto& us = UsbState::instance();
            std::string path = input.value("path", "");

            auto* dev = us.get_device(path);
            bool open  = dev && dev->is_Open();

            return Json{{"path", path},
                        {"is_open", open}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Check whether a USB device is currently open."});

    // -----------------------------------------------------------------------
    // 5. get_device_list
    // -----------------------------------------------------------------------
    app.tool(
        "get_device_list",
        make_object_schema({
            {"path", prop_string("Device path (omitted = list all managed devices)", "")},
        }),
        [](const Json& input) -> Json
        {
            auto& us = UsbState::instance();
            std::string path = input.value("path", "");

            if (!path.empty())
            {
                auto* dev = resolve_device(path, false);
                if (!dev)
                    throw std::runtime_error("Device not found: " + path);

                auto list = dev->get_Device_List();
                Json arr  = Json::array();
                for (const auto& d : list)
                    arr.push_back(device_info_to_json(d));
                return make_content(arr);
            }
            else
            {
                // List all managed devices with status
                auto paths = us.get_device_paths();
                Json arr   = Json::array();

                for (const auto& p : paths)
                {
                    auto* dev = us.get_device(p);
                    if (dev)
                        arr.push_back(build_device_status_json(*dev, p));
                }

                return Json{
                    {"managed_devices", arr},
                    {"count",            paths.size()}};
            }
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Get the cached device list. If path is provided, returns the device info list "
            "from that device; otherwise lists all managed devices with their status."});

    // -----------------------------------------------------------------------
    // 6. get_device_interfaces
    // -----------------------------------------------------------------------
    app.tool(
        "get_device_interfaces",
        make_object_schema({
            {"path", prop_string("Device path of the open USB device")},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            auto* dev = resolve_device(path);

            auto ifaces = dev->get_Device_Interfaces();

            Json arr = Json::array();
            for (const auto& iface : ifaces)
                arr.push_back(interface_info_to_json(iface));

            return make_content(arr);
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Get all interfaces of the currently opened USB device. "
            "Includes interface class, subclass, protocol, and endpoints."});

    // -----------------------------------------------------------------------
    // 7. get_interface_endpoints
    // -----------------------------------------------------------------------
    app.tool(
        "get_interface_endpoints",
        make_object_schema({
            {"path",             prop_string("Device path of the open USB device")},
            {"interface_number", prop_integer("Interface number to query", 0)},
        }, {"path", "interface_number"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            int iface_num    = input.value("interface_number", 0);
            auto* dev = resolve_device(path);

            auto endpoints = dev->get_Interface_Endpoints(iface_num);

            Json arr = Json::array();
            for (const auto& ep : endpoints)
                arr.push_back(endpoint_info_to_json(ep));

            Json text_entry;
            text_entry["type"] = "text";
            text_entry["text"] = "Interface " + std::to_string(iface_num) + " has " +
                                 std::to_string(endpoints.size()) +
                                 " endpoint(s).\n\n" + arr.dump(2);

            return Json{{"content", Json::array({text_entry})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Get all endpoints for a specific interface of the opened USB device. "
            "Each endpoint shows address, direction, transfer type, and max packet size."});

    // -----------------------------------------------------------------------
    // 8. find_endpoints_by_type
    // -----------------------------------------------------------------------
    app.tool(
        "find_endpoints_by_type",
        make_object_schema({
            {"path",          prop_string("Device path of the open USB device")},
            {"transfer_type", prop_string("Transfer type: bulk, interrupt, isochronous, control", "bulk")},
        }, {"path", "transfer_type"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            std::string type_str = input.value("transfer_type", "bulk");
            auto* dev = resolve_device(path);

            auto ttype = parse_transfer_type(type_str);
            auto endpoints = dev->find_Endpoints_By_Type(ttype);

            Json arr = Json::array();
            for (const auto& ep : endpoints)
                arr.push_back(endpoint_info_to_json(ep));

            Json text_entry;
            text_entry["type"] = "text";
            text_entry["text"] = "Found " + std::to_string(endpoints.size()) + " " +
                                 std::string(transfer_type_name(ttype)) +
                                 " endpoint(s).\n\n" + arr.dump(2);

            return Json{{"content", Json::array({text_entry})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Find all endpoints of a specific transfer type (bulk, interrupt, "
            "isochronous, or control) on the opened USB device."});

    // -----------------------------------------------------------------------
    // 9. auto_discover_endpoints
    // -----------------------------------------------------------------------
    app.tool(
        "auto_discover_endpoints",
        make_object_schema({
            {"path",          prop_string("Device path of the open USB device")},
            {"transfer_type", prop_string("Transfer type: bulk, interrupt, isochronous", "bulk")},
        }, {"path", "transfer_type"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            std::string type_str = input.value("transfer_type", "bulk");
            auto* dev = resolve_device(path);

            auto ttype = parse_transfer_type(type_str);
            uint8_t out_ep = 0, in_ep = 0;

            bool ok = dev->auto_Discover_Endpoints(ttype, out_ep, in_ep);

            if (!ok)
            {
                return Json{{"message",
                             "Could not auto-discover " +
                                 std::string(transfer_type_name(ttype)) +
                                 " endpoints. Try find_endpoints_by_type manually."},
                            {"path", path},
                            {"success", false}};
            }

            return Json{
                {"message", "Auto-discovered endpoints"},
                {"path", path},
                {"success", true},
                {"transfer_type", transfer_type_name(ttype)},
                {"out_endpoint", out_ep},
                {"out_endpoint_hex", "0x" +
                    [](uint8_t v) {
                        char buf[5];
                        snprintf(buf, sizeof(buf), "%02X", v);
                        return std::string(buf);
                    }(out_ep)},
                {"in_endpoint", in_ep},
                {"in_endpoint_hex", "0x" +
                    [](uint8_t v) {
                        char buf[5];
                        snprintf(buf, sizeof(buf), "%02X", v);
                        return std::string(buf);
                    }(in_ep)},
            };
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Auto-discover IN and OUT endpoints for a given transfer type. "
            "Returns the endpoint addresses ready for use in bulk/interrupt transfers. "
            "Use this after opening a device and before performing I/O."});

    // -----------------------------------------------------------------------
    // 10. claim_interface
    // -----------------------------------------------------------------------
    app.tool(
        "claim_interface",
        make_object_schema({
            {"path",             prop_string("Device path of the open USB device")},
            {"interface_number", prop_integer("Interface number to claim", 0)},
        }, {"path", "interface_number"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            int iface_num    = input.value("interface_number", 0);
            auto* dev = resolve_device(path);

            if (!dev->claim_Interface(iface_num))
                throw std::runtime_error("Failed to claim interface " +
                                         std::to_string(iface_num) + " on " + path);

            return Json{{"message", "Claimed interface " + std::to_string(iface_num)},
                        {"path", path},
                        {"interface_number", iface_num}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Claim a USB interface. Required before performing transfers on that "
            "interface's endpoints. Call release_interface when done."});

    // -----------------------------------------------------------------------
    // 11. release_interface
    // -----------------------------------------------------------------------
    app.tool(
        "release_interface",
        make_object_schema({
            {"path",             prop_string("Device path of the open USB device")},
            {"interface_number", prop_integer("Interface number to release", 0)},
        }, {"path", "interface_number"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            int iface_num    = input.value("interface_number", 0);
            auto* dev = resolve_device(path);

            if (!dev->release_Interface(iface_num))
                throw std::runtime_error("Failed to release interface " +
                                         std::to_string(iface_num) + " on " + path);

            return Json{{"message", "Released interface " + std::to_string(iface_num)},
                        {"path", path},
                        {"interface_number", iface_num}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Release a previously claimed USB interface. "
            "Should be called before closing the device."});

    // -----------------------------------------------------------------------
    // 12. control_transfer
    // -----------------------------------------------------------------------
    app.tool(
        "control_transfer",
        make_object_schema({
            {"path",            prop_string("Device path of the open USB device")},
            {"bmRequestType",   prop_integer("Request type bitmap (e.g. 0x80 for device-to-host standard)", 0x80)},
            {"bRequest",        prop_integer("Request code (e.g. 0x06 for GET_DESCRIPTOR)", 0x06)},
            {"wValue",          prop_integer("Value field (e.g. 0x0100 for device descriptor)", 0x0100)},
            {"wIndex",          prop_integer("Index field (e.g. 0x0000)", 0x0000)},
            {"data",            prop_string("Hex data to send (IN transfer = empty or length as bytes)", "")},
            {"data_length",     prop_integer("Data length for IN transfers (when data is empty)", 64)},
            {"timeout",         prop_integer("Timeout in milliseconds", 1000)},
        }, {"path", "bmRequestType", "bRequest", "wValue", "wIndex"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            uint8_t bmReq    = static_cast<uint8_t>(input.value("bmRequestType", 0x80));
            uint8_t bReq     = static_cast<uint8_t>(input.value("bRequest", 0x06));
            uint16_t wVal    = static_cast<uint16_t>(input.value("wValue", 0x0100));
            uint16_t wIdx    = static_cast<uint16_t>(input.value("wIndex", 0x0000));
            std::string data_hex = input.value("data", "");
            int data_len     = input.value("data_length", 64);
            unsigned int timeout = static_cast<unsigned int>(input.value("timeout", 1000));

            auto* dev = resolve_device(path);

            std::vector<uint8_t> data;
            if (!data_hex.empty())
            {
                data = hex_to_bytes(data_hex);
            }
            else if ((bmReq & 0x80) != 0) // IN direction
            {
                data.resize(data_len);
            }

            int transferred = dev->control_Transfer(bmReq, bReq, wVal, wIdx, data, timeout);

            if (transferred < 0)
                throw std::runtime_error("Control transfer failed with error " +
                                         std::to_string(transferred));

            return Json{
                {"message", "Control transfer completed"},
                {"path", path},
                {"bytes_transferred", transferred},
                {"data_hex", bytes_to_hex(data)},
                {"data_utf8", bytes_to_utf8_safe(data)},
            };
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Perform a USB control transfer. For IN transfers (bmRequestType bit 7 = 1), "
            "provide data_length to receive. For OUT transfers, provide hex data. "
            "Common request types: 0x80 (device-to-host), 0x00 (host-to-device)."});

    // -----------------------------------------------------------------------
    // 13. bulk_transfer
    // -----------------------------------------------------------------------
    app.tool(
        "bulk_transfer",
        make_object_schema({
            {"path",        prop_string("Device path of the open USB device")},
            {"endpoint",    prop_integer("Endpoint address (e.g. 0x01=OUT, 0x81=IN)", 0x81)},
            {"data",        prop_string("Hex data to write (empty = read)", "")},
            {"read_length", prop_integer("Number of bytes to read (when data is empty)", 64)},
            {"timeout",     prop_integer("Timeout in milliseconds", 1000)},
        }, {"path", "endpoint"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            uint8_t ep       = static_cast<uint8_t>(input.value("endpoint", 0x81));
            std::string data_hex = input.value("data", "");
            int read_len     = input.value("read_length", 64);
            unsigned int timeout = static_cast<unsigned int>(input.value("timeout", 1000));

            auto* dev = resolve_device(path);

            bool is_in = (ep & 0x80) != 0;
            std::vector<uint8_t> data;

            if (!data_hex.empty())
            {
                data = hex_to_bytes(data_hex);
            }
            else
            {
                data.resize(read_len);
            }

            int transferred = dev->bulk_Transfer(ep, data, timeout);

            if (transferred < 0)
                throw std::runtime_error("Bulk transfer failed with error " +
                                         std::to_string(transferred));

            return Json{
                {"message", "Bulk transfer completed"},
                {"path", path},
                {"endpoint", ep},
                {"direction", is_in ? "IN" : "OUT"},
                {"bytes_transferred", transferred},
                {"data_hex", bytes_to_hex(data)},
                {"data_utf8", bytes_to_utf8_safe(data)},
            };
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Perform a USB bulk transfer. For IN transfers (endpoint bit 7 = 1, e.g. 0x81), "
            "provide read_length to receive data. For OUT transfers (bit 7 = 0, e.g. 0x01), "
            "provide hex data. The interface must be claimed first."});

    // -----------------------------------------------------------------------
    // 14. interrupt_transfer
    // -----------------------------------------------------------------------
    app.tool(
        "interrupt_transfer",
        make_object_schema({
            {"path",        prop_string("Device path of the open USB device")},
            {"endpoint",    prop_integer("Endpoint address (e.g. 0x81=IN)", 0x81)},
            {"data",        prop_string("Hex data to write (empty = read)", "")},
            {"read_length", prop_integer("Number of bytes to read (when data is empty)", 64)},
            {"timeout",     prop_integer("Timeout in milliseconds", 1000)},
        }, {"path", "endpoint"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            uint8_t ep       = static_cast<uint8_t>(input.value("endpoint", 0x81));
            std::string data_hex = input.value("data", "");
            int read_len     = input.value("read_length", 64);
            unsigned int timeout = static_cast<unsigned int>(input.value("timeout", 1000));

            auto* dev = resolve_device(path);

            bool is_in = (ep & 0x80) != 0;
            std::vector<uint8_t> data;

            if (!data_hex.empty())
            {
                data = hex_to_bytes(data_hex);
            }
            else
            {
                data.resize(read_len);
            }

            int transferred = dev->interrupt_Transfer(ep, data, timeout);

            if (transferred < 0)
                throw std::runtime_error("Interrupt transfer failed with error " +
                                         std::to_string(transferred));

            return Json{
                {"message", "Interrupt transfer completed"},
                {"path", path},
                {"endpoint", ep},
                {"direction", is_in ? "IN" : "OUT"},
                {"bytes_transferred", transferred},
                {"data_hex", bytes_to_hex(data)},
                {"data_utf8", bytes_to_utf8_safe(data)},
            };
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Perform a USB interrupt transfer. Works the same as bulk_transfer but for "
            "interrupt endpoints (typically used for HID devices). The interface must be claimed first."});

    // -----------------------------------------------------------------------
    // 15. isochronous_transfer
    // -----------------------------------------------------------------------
    app.tool(
        "isochronous_transfer",
        make_object_schema({
            {"path",            prop_string("Device path of the open USB device")},
            {"endpoint",        prop_integer("Endpoint address (e.g. 0x81=IN, 0x01=OUT)", 0x81)},
            {"data",            prop_string("Hex data to write (empty = read)", "")},
            {"num_packets",     prop_integer("Number of isochronous packets", 1)},
            {"packet_length",   prop_integer("Length of each packet in bytes (used if packet_lengths not provided)", 512)},
            {"packet_lengths",  prop_array("Array of per-packet lengths (overrides packet_length)")},
            {"timeout",         prop_integer("Timeout in milliseconds", 1000)},
        }, {"path", "endpoint", "num_packets"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            uint8_t ep       = static_cast<uint8_t>(input.value("endpoint", 0x81));
            std::string data_hex = input.value("data", "");
            int num_pkts     = input.value("num_packets", 1);
            int pkt_len      = input.value("packet_length", 512);
            unsigned int timeout = static_cast<unsigned int>(input.value("timeout", 1000));

            auto* dev = resolve_device(path);

            bool is_in = (ep & 0x80) != 0;

            // Build packet lengths
            std::vector<int> pkt_lengths;
            if (input.contains("packet_lengths") && input["packet_lengths"].is_array())
            {
                for (const auto& v : input["packet_lengths"])
                    pkt_lengths.push_back(v.get<int>());
            }
            else
            {
                pkt_lengths.assign(num_pkts, pkt_len);
            }

            // Build data buffer
            std::vector<uint8_t> data;
            if (!data_hex.empty())
            {
                data = hex_to_bytes(data_hex);
            }
            else if (is_in)
            {
                int total = 0;
                for (int len : pkt_lengths)
                    total += len;
                data.resize(total);
            }
            else
            {
                // For OUT, size buffer to match packet lengths total
                int total = 0;
                for (int len : pkt_lengths)
                    total += len;
                if (total > 0)
                    data.resize(total);
            }

            std::vector<SimpleCommKit::SimpleCommKitUsbIsoPacketResult> results;
            int transferred = dev->isochronous_Transfer(ep, data, num_pkts, pkt_lengths, results, timeout);

            if (transferred < 0)
                throw std::runtime_error("Isochronous transfer failed with error " +
                                         std::to_string(transferred));

            Json pkt_json = Json::array();
            for (const auto& r : results)
                pkt_json.push_back(iso_packet_to_json(r));

            return Json{
                {"message", "Isochronous transfer completed"},
                {"path", path},
                {"endpoint", ep},
                {"direction", is_in ? "IN" : "OUT"},
                {"total_bytes_transferred", transferred},
                {"packet_count", num_pkts},
                {"packet_results", pkt_json},
                {"data_hex", bytes_to_hex(data)},
            };
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Perform a USB isochronous transfer (synchronous wrapper). "
            "Used for streaming data like audio/video. Specify num_packets and "
            "packet_length(s) to control the transfer. The interface must be claimed first."});

    // -----------------------------------------------------------------------
    // 16. start_read_poll
    // -----------------------------------------------------------------------
    app.tool(
        "start_read_poll",
        make_object_schema({
            {"path",     prop_string("Device path of the open USB device")},
            {"endpoint", prop_integer("IN endpoint address to poll (e.g. 0x81)", 0x81)},
        }, {"path", "endpoint"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            uint8_t ep       = static_cast<uint8_t>(input.value("endpoint", 0x81));
            auto* dev = resolve_device(path);

            // Ensure read callback is installed
            auto& us = UsbState::instance();
            us.setup_read_callback(path);

            dev->start_Read_Poll(ep);

            return Json{{"message",
                         "Started read polling on endpoint 0x" +
                             [](uint8_t v) {
                                 char buf[5];
                                 snprintf(buf, sizeof(buf), "%02X", v);
                                 return std::string(buf);
                             }(ep)},
                        {"path", path},
                        {"endpoint", ep}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Start continuous read polling on a specific IN endpoint. "
            "Data is buffered and can be retrieved with get_read_data. "
            "Use stop_read_poll to stop. The interface must be claimed first."});

    // -----------------------------------------------------------------------
    // 17. stop_read_poll
    // -----------------------------------------------------------------------
    app.tool(
        "stop_read_poll",
        make_object_schema({
            {"path", prop_string("Device path of the open USB device")},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            auto* dev = resolve_device(path);

            if (!dev->is_Read_Poll_Active())
                return Json{{"message", "Read poll is not active on " + path},
                            {"path", path}};

            dev->stop_Read_Poll();

            return Json{{"message", "Stopped read polling on " + path},
                        {"path", path}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Stop the continuous read polling on a device. "
            "Any remaining buffered data can still be retrieved with get_read_data."});

    // -----------------------------------------------------------------------
    // 18. get_read_data
    // -----------------------------------------------------------------------
    app.tool(
        "get_read_data",
        make_object_schema({
            {"path", prop_string("Device path to retrieve read data for")},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            auto& us = UsbState::instance();
            std::string path = input.value("path", "");

            auto entries = us.drain_read_buffer(path);

            Json arr = Json::array();
            for (const auto& e : entries)
            {
                arr.push_back(Json{{"device_path", e.device_path},
                                   {"vendor_id", e.vendor_id},
                                   {"product_id", e.product_id},
                                   {"data_hex", e.data_hex},
                                   {"data_utf8", e.data_utf8},
                                   {"data_length", e.data_length}});
            }
            return make_content(arr);
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Retrieve and clear buffered read data for a USB device. "
            "Data is buffered automatically when read polling is active."});

    // -----------------------------------------------------------------------
    // 19. start_hotplug
    // -----------------------------------------------------------------------
    app.tool(
        "start_hotplug",
        make_object_schema({
            {"path",       prop_string("Device path of the open USB device")},
            {"vendor_id",  prop_integer("Filter by vendor ID (0 = all)", 0)},
            {"product_id", prop_integer("Filter by product ID (0 = all)", 0)},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            unsigned short vid = static_cast<unsigned short>(
                input.value("vendor_id", 0));
            unsigned short pid = static_cast<unsigned short>(
                input.value("product_id", 0));
            auto* dev = resolve_device(path);

            // Ensure hotplug callback is installed
            auto& us = UsbState::instance();
            us.setup_hotplug_callback(path);

            dev->start_Hotplug(vid, pid);

            return Json{{"message", "Started hotplug detection on " + path},
                        {"path", path},
                        {"vendor_id", vid},
                        {"product_id", pid}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Start polling-based USB hotplug detection. "
            "Events (device attached/detached) are buffered and can be "
            "retrieved with get_hotplug_data. Use stop_hotplug to stop."});

    // -----------------------------------------------------------------------
    // 20. stop_hotplug
    // -----------------------------------------------------------------------
    app.tool(
        "stop_hotplug",
        make_object_schema({
            {"path", prop_string("Device path of the open USB device")},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            auto* dev = resolve_device(path);

            if (!dev->is_Hotplug_Active())
                return Json{{"message", "Hotplug detection is not active on " + path},
                            {"path", path}};

            dev->stop_Hotplug();

            return Json{{"message", "Stopped hotplug detection on " + path},
                        {"path", path}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Stop polling-based USB hotplug detection. "
            "Any remaining events can still be retrieved with get_hotplug_data."});

    // -----------------------------------------------------------------------
    // 21. get_hotplug_data
    // -----------------------------------------------------------------------
    app.tool(
        "get_hotplug_data",
        [](const Json&) -> Json
        {
            auto& us = UsbState::instance();

            // Hotplug events are stored under the global key "_global_"
            auto entries = us.drain_hotplug_buffer("_global_");

            Json attached   = Json::array();
            Json detached   = Json::array();

            for (const auto& e : entries)
            {
                Json entry = Json{
                    {"device_path",         e.device_path},
                    {"vendor_id",           e.vendor_id},
                    {"product_id",          e.product_id},
                    {"manufacturer_string", e.manufacturer_string},
                    {"product_string",      e.product_string},
                    {"serial_number",       e.serial_number},
                };
                if (e.is_attached)
                    attached.push_back(entry);
                else
                    detached.push_back(entry);
            }

            return Json{
                {"total_events", entries.size()},
                {"attached", attached},
                {"attached_count", attached.size()},
                {"detached", detached},
                {"detached_count", detached.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Retrieve and clear buffered USB hotplug events. "
            "Returns lists of attached and detached devices since "
            "the last call. Requires start_hotplug to be active on at least one device."});

    // -----------------------------------------------------------------------
    // 22. set_read_poll_interval
    // -----------------------------------------------------------------------
    app.tool(
        "set_read_poll_interval",
        make_object_schema({
            {"path",  prop_string("Device path of the open USB device")},
            {"ms",    prop_integer("Poll interval in milliseconds (default 100)", 100)},
            {"length", prop_integer("Read data length per poll (default 64, 0 = no change)", 0)},
        }, {"path", "ms"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            int ms           = input.value("ms", 100);
            int length       = input.value("length", 0);
            auto* dev = resolve_device(path);

            dev->set_Read_Poll_Interval(ms);
            if (length > 0)
                dev->set_Read_Data_Length(length);

            return Json{{"message", "Read poll interval updated"},
                        {"path", path},
                        {"read_poll_interval_ms", dev->get_Read_Poll_Interval()},
                        {"read_data_length", dev->get_Read_Data_Length()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Configure the read poll interval and optional data length for a USB device. "
            "Changes take effect immediately if read polling is active."});

    // -----------------------------------------------------------------------
    // 23. get_error
    // -----------------------------------------------------------------------
    app.tool(
        "get_error",
        make_object_schema({
            {"path", prop_string("Device path of the open USB device")},
        }, {"path"}),
        [](const Json& input) -> Json
        {
            std::string path = input.value("path", "");
            auto* dev = resolve_device(path, false);

            // SimpleCommKitUsb doesn't have a direct get_Last_Error, but we can
            // use the error callback. Return success if device exists.
            return build_device_status_json(*dev, path);
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Get the current status of a USB device including open state, "
            "read poll status, hotplug status, and configuration parameters."});
}

} // namespace SimpleCommKitAiUsbFastmcpp
