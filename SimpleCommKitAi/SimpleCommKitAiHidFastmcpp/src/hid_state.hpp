#pragma once

#include <SimpleCommKitHid.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace SimpleCommKitAiHidFastmcpp
{

using Json = nlohmann::json;

/// Read-data entry buffered by the background read thread.
struct ReadDataEntry
{
    std::string path;
    std::string data_hex;
    std::string data_utf8;
    size_t data_length = 0;
};

/// Singleton that holds HID state shared across all MCP tools.
/// Mirrors the Python `HidState` class in SimpleCommKitAiHid.
class HidState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static HidState& instance();

    /// Lazy-initialize the HID subsystem. Safe to call multiple times.
    /// @return true if HID hardware is ready.
    bool init();

    /// Ensure HID is initialized; throws std::runtime_error on failure.
    SimpleCommKit::SimpleCommKitHid& ensure_hid();

    /// Install the global read callback. Buffers data keyed by device path.
    /// Safe to call multiple times (idempotent — only first call takes effect).
    void setup_read_callback();

    // -----------------------------------------------------------------------
    // Read buffer access (thread-safe)
    // -----------------------------------------------------------------------

    /// Retrieve and clear all buffered read entries for a device path.
    std::vector<ReadDataEntry> drain_read_buffer(const std::string& path);

    /// Remove a device's read buffer (called on close).
    void remove_read_buffer(const std::string& path);

    /// Clear all read buffers (called on close-all).
    void clear_read_buffers();

    // -----------------------------------------------------------------------
    // Device cache
    // -----------------------------------------------------------------------

    std::vector<SimpleCommKit::SimpleCommKitHidDeviceInfo> device_cache;
    std::atomic<bool> hotplug_active{false};

  private:
    HidState() = default;
    ~HidState() = default;
    HidState(const HidState&) = delete;
    HidState& operator=(const HidState&) = delete;

    std::unique_ptr<SimpleCommKit::SimpleCommKitHid> hid_;
    bool initialized_ = false;
    std::mutex mutex_;

    /// path → buffered read entries
    std::unordered_map<std::string, std::vector<ReadDataEntry>> read_buffer_;
    std::mutex read_buffer_mutex_;
};

/// Convert a hex string (e.g. "00FF" or "01 02 AA") to a byte vector.
/// Throws std::runtime_error on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

/// Convert a byte vector to a hex string (no spaces).
std::string bytes_to_hex(const std::vector<uint8_t>& data);

/// Try to interpret binary data as UTF-8; invalid bytes are skipped.
std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data);

/// Convert a HidDeviceInfo struct to a JSON object (same fields as Python).
Json device_info_to_json(const SimpleCommKit::SimpleCommKitHidDeviceInfo& d);

/// Convert bus_type integer to human-readable name.
const char* bus_type_name(int bus_type);

} // namespace SimpleCommKitAiHidFastmcpp
