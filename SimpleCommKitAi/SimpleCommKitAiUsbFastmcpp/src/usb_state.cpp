#include "usb_state.hpp"

#include <SimpleCommKitErrorMap.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace SimpleCommKitAiUsbFastmcpp
{

// ===========================================================================
// UsbState singleton
// ===========================================================================

UsbState& UsbState::instance()
{
    static UsbState s;
    return s;
}

SimpleCommKit::SimpleCommKitUsb* UsbState::get_or_create_device(const std::string& path)
{
    std::lock_guard<std::mutex> lock(devices_mutex_);
    auto it = devices_.find(path);
    if (it != devices_.end())
        return it->second.get();

    auto dev = std::make_unique<SimpleCommKit::SimpleCommKitUsb>();
    auto* raw = dev.get();
    devices_[path] = std::move(dev);
    return raw;
}

bool UsbState::has_device(const std::string& path)
{
    std::lock_guard<std::mutex> lock(devices_mutex_);
    return devices_.find(path) != devices_.end();
}

SimpleCommKit::SimpleCommKitUsb* UsbState::get_device(const std::string& path)
{
    std::lock_guard<std::mutex> lock(devices_mutex_);
    auto it = devices_.find(path);
    if (it == devices_.end())
        return nullptr;
    return it->second.get();
}

void UsbState::remove_device(const std::string& path)
{
    {
        std::lock_guard<std::mutex> lock(devices_mutex_);
        auto it = devices_.find(path);
        if (it != devices_.end())
        {
            if (it->second && it->second->is_Open())
                it->second->close();
            devices_.erase(it);
        }
    }
    remove_read_buffer(path);
    remove_hotplug_buffer(path);
}

std::vector<std::string> UsbState::get_device_paths()
{
    std::lock_guard<std::mutex> lock(devices_mutex_);
    std::vector<std::string> paths;
    paths.reserve(devices_.size());
    for (const auto& kv : devices_)
        paths.push_back(kv.first);
    return paths;
}

void UsbState::remove_all_devices()
{
    {
        std::lock_guard<std::mutex> lock(devices_mutex_);
        for (auto& kv : devices_)
        {
            if (kv.second)
            {
                if (kv.second->is_Read_Poll_Active())
                    kv.second->stop_Read_Poll();
                if (kv.second->is_Hotplug_Active())
                    kv.second->stop_Hotplug();
                if (kv.second->is_Open())
                    kv.second->close();
            }
        }
        devices_.clear();
    }
    clear_read_buffers();
    clear_hotplug_buffers();
}

void UsbState::setup_read_callback(const std::string& path)
{
    auto* dev = get_device(path);
    if (!dev)
        return;

    dev->set_Callback_On_Read(
        [this, path](const SimpleCommKit::SimpleCommKitUsbDeviceInfo& info,
                      const std::vector<uint8_t>& data)
        {
            UsbReadEntry entry;
            entry.device_path = path;
            entry.vendor_id   = info.vendor_id;
            entry.product_id  = info.product_id;
            entry.data_hex    = bytes_to_hex(data);
            entry.data_utf8   = bytes_to_utf8_safe(data);
            entry.data_length = data.size();

            std::lock_guard<std::mutex> lock(read_buffer_mutex_);
            read_buffer_[path].push_back(std::move(entry));
        });

    // Register error callback
    dev->set_Callback_Error(
        [path](SimpleCommKit::ErrorCode code)
        {
            std::string desc =
                SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(code);
            std::cerr << "[SimpleCommKitAiUsbFastmcpp] Error on " << path
                      << ": " << code << " - " << desc << std::endl;
        });
}

void UsbState::setup_hotplug_callback(const std::string& path)
{
    auto* dev = get_device(path);
    if (!dev)
        return;

    dev->set_Callback_On_HotPlug(
        [this](const std::vector<SimpleCommKit::SimpleCommKitUsbDeviceInfo>& added,
               const std::vector<SimpleCommKit::SimpleCommKitUsbDeviceInfo>& removed)
        {
            std::lock_guard<std::mutex> lock(hotplug_buffer_mutex_);

            for (const auto& info : added)
            {
                UsbHotplugEntry entry;
                entry.device_path          = info.path;
                entry.vendor_id            = info.vendor_id;
                entry.product_id           = info.product_id;
                entry.manufacturer_string  = info.manufacturer_string;
                entry.product_string       = info.product_string;
                entry.serial_number        = info.serial_number;
                entry.is_attached          = true;
                hotplug_buffer_["_global_"].push_back(std::move(entry));
            }

            for (const auto& info : removed)
            {
                UsbHotplugEntry entry;
                entry.device_path          = info.path;
                entry.vendor_id            = info.vendor_id;
                entry.product_id           = info.product_id;
                entry.manufacturer_string  = info.manufacturer_string;
                entry.product_string       = info.product_string;
                entry.serial_number        = info.serial_number;
                entry.is_attached          = false;
                hotplug_buffer_["_global_"].push_back(std::move(entry));
            }
        });
}

// ===========================================================================
// Read buffer
// ===========================================================================

std::vector<UsbReadEntry> UsbState::drain_read_buffer(const std::string& path)
{
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);
    auto it = read_buffer_.find(path);
    if (it == read_buffer_.end())
        return {};
    auto entries = std::move(it->second);
    it->second.clear();
    return entries;
}

void UsbState::remove_read_buffer(const std::string& path)
{
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);
    read_buffer_.erase(path);
}

void UsbState::clear_read_buffers()
{
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);
    read_buffer_.clear();
}

// ===========================================================================
// Hotplug buffer
// ===========================================================================

std::vector<UsbHotplugEntry> UsbState::drain_hotplug_buffer(const std::string& path)
{
    std::lock_guard<std::mutex> lock(hotplug_buffer_mutex_);
    auto it = hotplug_buffer_.find(path);
    if (it == hotplug_buffer_.end())
        return {};
    auto entries = std::move(it->second);
    it->second.clear();
    return entries;
}

void UsbState::remove_hotplug_buffer(const std::string& path)
{
    std::lock_guard<std::mutex> lock(hotplug_buffer_mutex_);
    hotplug_buffer_.erase(path);
}

void UsbState::clear_hotplug_buffers()
{
    std::lock_guard<std::mutex> lock(hotplug_buffer_mutex_);
    hotplug_buffer_.clear();
}

// ===========================================================================
// Utility functions
// ===========================================================================

std::vector<uint8_t> hex_to_bytes(const std::string& hex)
{
    // Remove whitespace
    std::string cleaned;
    cleaned.reserve(hex.size());
    for (char c : hex)
    {
        if (!std::isspace(static_cast<unsigned char>(c)))
            cleaned.push_back(c);
    }

    if (cleaned.size() % 2 != 0)
        throw std::runtime_error("Hex string must have an even number of characters");

    std::vector<uint8_t> bytes;
    bytes.reserve(cleaned.size() / 2);

    for (size_t i = 0; i < cleaned.size(); i += 2)
    {
        char high = cleaned[i];
        char low  = cleaned[i + 1];

        auto hex_val = [](char c) -> int
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            throw std::runtime_error(std::string("Invalid hex character: ") + c);
        };

        bytes.push_back(static_cast<uint8_t>((hex_val(high) << 4) | hex_val(low)));
    }

    return bytes;
}

std::string bytes_to_hex(const std::vector<uint8_t>& data)
{
    static const char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(data.size() * 2);
    for (uint8_t b : data)
    {
        result.push_back(hex_chars[b >> 4]);
        result.push_back(hex_chars[b & 0x0F]);
    }
    return result;
}

std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data)
{
    std::string result;
    result.reserve(data.size());

    size_t i = 0;
    while (i < data.size())
    {
        uint8_t b = data[i];

        // ASCII (0xxxxxxx)
        if (b <= 0x7F)
        {
            result.push_back(static_cast<char>(b));
            ++i;
            continue;
        }

        // 2-byte sequence (110xxxxx 10xxxxxx)
        if ((b & 0xE0) == 0xC0 && i + 1 < data.size() && (data[i + 1] & 0xC0) == 0x80)
        {
            result.push_back(static_cast<char>(b));
            result.push_back(static_cast<char>(data[i + 1]));
            i += 2;
            continue;
        }

        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        if ((b & 0xF0) == 0xE0 && i + 2 < data.size() &&
            (data[i + 1] & 0xC0) == 0x80 && (data[i + 2] & 0xC0) == 0x80)
        {
            result.push_back(static_cast<char>(b));
            result.push_back(static_cast<char>(data[i + 1]));
            result.push_back(static_cast<char>(data[i + 2]));
            i += 3;
            continue;
        }

        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        if ((b & 0xF8) == 0xF0 && i + 3 < data.size() &&
            (data[i + 1] & 0xC0) == 0x80 && (data[i + 2] & 0xC0) == 0x80 &&
            (data[i + 3] & 0xC0) == 0x80)
        {
            result.push_back(static_cast<char>(b));
            result.push_back(static_cast<char>(data[i + 1]));
            result.push_back(static_cast<char>(data[i + 2]));
            result.push_back(static_cast<char>(data[i + 3]));
            i += 4;
            continue;
        }

        // Skip invalid byte
        ++i;
    }

    return result;
}

// ===========================================================================
// JSON converters
// ===========================================================================

Json device_info_to_json(const SimpleCommKit::SimpleCommKitUsbDeviceInfo& info)
{
    return Json{
        {"vendor_id",            info.vendor_id},
        {"product_id",           info.product_id},
        {"manufacturer_string",  info.manufacturer_string},
        {"product_string",       info.product_string},
        {"serial_number",        info.serial_number},
        {"bus_number",           info.bus_number},
        {"device_address",       info.device_address},
        {"path",                 info.path},
    };
}

Json endpoint_info_to_json(const SimpleCommKit::SimpleCommKitUsbEndpointInfo& ep)
{
    return Json{
        {"endpoint_address", ep.endpoint_address},
        {"attributes",       ep.attributes},
        {"max_packet_size",  ep.max_packet_size},
        {"interval",         ep.interval},
        {"is_in",            ep.is_in},
        {"direction",        ep.is_in ? "IN" : "OUT"},
        {"is_bulk",          ep.isBulk()},
        {"is_interrupt",     ep.isInterrupt()},
        {"is_isochronous",   ep.isIsochronous()},
        {"is_control",       ep.isControl()},
    };
}

Json interface_info_to_json(const SimpleCommKit::SimpleCommKitUsbInterfaceInfo& iface)
{
    Json eps = Json::array();
    for (const auto& ep : iface.endpoints)
        eps.push_back(endpoint_info_to_json(ep));

    return Json{
        {"interface_number",  iface.interface_number},
        {"alternate_setting", iface.alternate_setting},
        {"num_endpoints",     iface.num_endpoints},
        {"interface_class",   iface.interface_class},
        {"interface_subclass", iface.interface_subclass},
        {"interface_protocol", iface.interface_protocol},
        {"interface_string",  iface.interface_string},
        {"endpoints",         eps},
    };
}

Json iso_packet_to_json(const SimpleCommKit::SimpleCommKitUsbIsoPacketResult& pkt)
{
    return Json{
        {"length",        pkt.length},
        {"actual_length", pkt.actual_length},
        {"status",        pkt.status},
    };
}

const char* transfer_type_name(SimpleCommKit::SimpleCommKitUsbTransferType type)
{
    switch (type)
    {
    case SimpleCommKit::SimpleCommKitUsbTransferType::Control:
        return "control";
    case SimpleCommKit::SimpleCommKitUsbTransferType::Isochronous:
        return "isochronous";
    case SimpleCommKit::SimpleCommKitUsbTransferType::Bulk:
        return "bulk";
    case SimpleCommKit::SimpleCommKitUsbTransferType::Interrupt:
        return "interrupt";
    default:
        return "unknown";
    }
}

SimpleCommKit::SimpleCommKitUsbTransferType parse_transfer_type(const std::string& s)
{
    if (s == "control")
        return SimpleCommKit::SimpleCommKitUsbTransferType::Control;
    if (s == "isochronous")
        return SimpleCommKit::SimpleCommKitUsbTransferType::Isochronous;
    if (s == "bulk")
        return SimpleCommKit::SimpleCommKitUsbTransferType::Bulk;
    if (s == "interrupt")
        return SimpleCommKit::SimpleCommKitUsbTransferType::Interrupt;
    throw std::runtime_error("Unknown transfer type: " + s +
                             ". Valid: control, isochronous, bulk, interrupt");
}

} // namespace SimpleCommKitAiUsbFastmcpp
