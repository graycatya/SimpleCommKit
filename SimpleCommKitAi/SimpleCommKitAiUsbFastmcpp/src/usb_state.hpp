#pragma once

#include <SimpleCommKitUsb.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace SimpleCommKitAiUsbFastmcpp
{

using Json = nlohmann::json;

/// Read-data entry buffered by the background read-poll thread.
struct UsbReadEntry
{
    std::string device_path;
    uint16_t vendor_id   = 0;
    uint16_t product_id  = 0;
    std::string data_hex;
    std::string data_utf8;
    size_t data_length = 0;
};

/// Hotplug event entry buffered by the hotplug polling thread.
struct UsbHotplugEntry
{
    std::string device_path;
    uint16_t vendor_id  = 0;
    uint16_t product_id = 0;
    std::string manufacturer_string;
    std::string product_string;
    std::string serial_number;
    bool is_attached = true; // true = attached, false = detached
};

/// Singleton that holds USB state shared across all MCP tools.
/// Mirrors the SerialPort pattern: manages multiple independent USB device instances.
class UsbState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static UsbState& instance();

    /// Create or return the SimpleCommKitUsb for the given device path.
    /// The instance is created on first call and reused thereafter.
    SimpleCommKit::SimpleCommKitUsb* get_or_create_device(const std::string& path);

    /// Check whether a device with the given path is currently managed.
    bool has_device(const std::string& path);

    /// Get a managed device; returns nullptr if not found.
    SimpleCommKit::SimpleCommKitUsb* get_device(const std::string& path);

    /// Remove a managed device from the map and its read/hotplug buffers.
    void remove_device(const std::string& path);

    /// Get a list of all currently managed device paths.
    std::vector<std::string> get_device_paths();

    /// Remove all managed devices and clear all buffers.
    void remove_all_devices();

    /// Install the On_Read callback for the given device. Safe to call multiple
    /// times (idempotent).
    void setup_read_callback(const std::string& path);

    /// Install the On_HotPlug callback for the given device.
    void setup_hotplug_callback(const std::string& path);

    // -----------------------------------------------------------------------
    // Read buffer access (thread-safe)
    // -----------------------------------------------------------------------

    /// Retrieve and clear all buffered read entries for a device path.
    std::vector<UsbReadEntry> drain_read_buffer(const std::string& path);

    /// Remove a device's read buffer (called on close).
    void remove_read_buffer(const std::string& path);

    /// Clear all read buffers (called on close-all).
    void clear_read_buffers();

    // -----------------------------------------------------------------------
    // Hotplug buffer access (thread-safe)
    // -----------------------------------------------------------------------

    /// Retrieve and clear all buffered hotplug events for a device path.
    std::vector<UsbHotplugEntry> drain_hotplug_buffer(const std::string& path);

    /// Remove a device's hotplug buffer (called on close).
    void remove_hotplug_buffer(const std::string& path);

    /// Clear all hotplug buffers (called on close-all).
    void clear_hotplug_buffers();

  private:
    UsbState()  = default;
    ~UsbState() = default;
    UsbState(const UsbState&)            = delete;
    UsbState& operator=(const UsbState&) = delete;

    std::mutex devices_mutex_;
    /// devicePath -> SimpleCommKitUsb instance
    std::unordered_map<std::string, std::unique_ptr<SimpleCommKit::SimpleCommKitUsb>> devices_;

    /// devicePath -> buffered read entries
    std::unordered_map<std::string, std::vector<UsbReadEntry>> read_buffer_;
    std::mutex read_buffer_mutex_;

    /// devicePath -> buffered hotplug entries
    std::unordered_map<std::string, std::vector<UsbHotplugEntry>> hotplug_buffer_;
    std::mutex hotplug_buffer_mutex_;
};

/// Convert a hex string (e.g. "00FF" or "01 02 AA") to a byte vector.
/// Throws std::runtime_error on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

/// Convert a byte vector to a hex string (no spaces, lowercase).
std::string bytes_to_hex(const std::vector<uint8_t>& data);

/// Try to interpret binary data as UTF-8; invalid bytes are skipped.
std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data);

/// Convert a SimpleCommKitUsbDeviceInfo struct to a JSON object.
Json device_info_to_json(const SimpleCommKit::SimpleCommKitUsbDeviceInfo& info);

/// Convert a SimpleCommKitUsbEndpointInfo struct to a JSON object.
Json endpoint_info_to_json(const SimpleCommKit::SimpleCommKitUsbEndpointInfo& ep);

/// Convert a SimpleCommKitUsbInterfaceInfo struct to a JSON object.
Json interface_info_to_json(const SimpleCommKit::SimpleCommKitUsbInterfaceInfo& iface);

/// Convert a SimpleCommKitUsbIsoPacketResult struct to a JSON object.
Json iso_packet_to_json(const SimpleCommKit::SimpleCommKitUsbIsoPacketResult& pkt);

/// Transfer-type enum to string.
const char* transfer_type_name(SimpleCommKit::SimpleCommKitUsbTransferType type);

/// Parse a transfer-type string (e.g. "bulk") to the enum value.
SimpleCommKit::SimpleCommKitUsbTransferType parse_transfer_type(const std::string& s);

} // namespace SimpleCommKitAiUsbFastmcpp
