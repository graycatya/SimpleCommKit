#pragma once

#include <SimpleCommKitBleCentral.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SimpleCommKitAiBleFastmcpp
{

using Json = nlohmann::json;

/// Buffered notification/indication entry keyed by (address, characteristic_uuid).
struct NotificationEntry
{
    std::string address;
    std::string service_uuid;
    std::string characteristic_uuid;
    std::string data_hex;
    std::string data_utf8;
    size_t data_length = 0;
};

/// Peripheral info extended with connection state.
struct PeripheralInfo
{
    SimpleCommKit::SimpleCommKitBlePeripheral peripheral;
    bool connected = false;
};

/// Singleton that holds BLE state shared across all MCP tools.
/// Mirrors the Python `BleState` class in SimpleCommKitAiBle.
class BleState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static BleState& instance();

    /// Lazy-initialize the BLE subsystem. Safe to call multiple times.
    /// @return true if BLE hardware is ready.
    bool init();

    /// Ensure BLE is initialized; throws std::runtime_error on failure.
    SimpleCommKit::SimpleCommKitBleCentral& ensure_ble();

    /// Install notification/indication callbacks for a peripheral.
    /// Safe to call multiple times (idempotent per callback).
    void setup_notification_callbacks();

    // -----------------------------------------------------------------------
    // Adapter management (switching adapters)
    // -----------------------------------------------------------------------
    std::optional<SimpleCommKit::SimpleCommKitBleAdapter> current_adapter_;

    // -----------------------------------------------------------------------
    // Scan results cache
    // -----------------------------------------------------------------------
    std::vector<SimpleCommKit::SimpleCommKitBlePeripheral> scan_results;
    mutable std::mutex scan_results_mutex_;

    /// Add a found peripheral to scan results (dedup by address).
    void add_scan_result(const SimpleCommKit::SimpleCommKitBlePeripheral& p);

    /// Clear all scan results.
    void clear_scan_results();

    // -----------------------------------------------------------------------
    // Connected peripherals tracking
    // -----------------------------------------------------------------------
    /// address → PeripheralInfo
    std::unordered_map<std::string, PeripheralInfo> peripherals_;
    mutable std::mutex peripherals_mutex_;

    /// Mark a peripheral as connected (by address).
    void set_connected(const std::string& address, bool connected = true);

    /// Check if a peripheral is tracked as connected.
    bool is_connected(const std::string& address);

    // -----------------------------------------------------------------------
    // Notification buffer access (thread-safe)
    // -----------------------------------------------------------------------
    void push_notification(const std::string& address,
                           const std::string& service_uuid,
                           const std::string& characteristic_uuid,
                           const std::vector<uint8_t>& data);

    /// Retrieve and clear all buffered notifications for a device address.
    std::vector<NotificationEntry> drain_notifications(const std::string& address);

    /// Remove a device's notification buffer (called on disconnect).
    void remove_notification_buffer(const std::string& address);

    /// Clear all notification buffers (called on cleanup).
    void clear_notification_buffers();

    // -----------------------------------------------------------------------
    //  Shutdown
    // -----------------------------------------------------------------------

    /// Disconnect all peripherals, unsubscribe, and release BLE resources.
    /// Call before application exit to avoid blocking on active connections.
    void cleanup();

    // -----------------------------------------------------------------------
    // Subscriptions tracking
    // -----------------------------------------------------------------------
    struct SubscriptionKey
    {
        std::string service_uuid;
        std::string characteristic_uuid;
        bool operator==(const SubscriptionKey& other) const
        {
            return service_uuid == other.service_uuid &&
                   characteristic_uuid == other.characteristic_uuid;
        }
    };

    struct SubscriptionKeyHash
    {
        size_t operator()(const SubscriptionKey& k) const
        {
            return std::hash<std::string>{}(k.service_uuid + "::" + k.characteristic_uuid);
        }
    };

    /// address → set of subscribed (service, characteristic) pairs.
    std::unordered_map<std::string, std::unordered_set<SubscriptionKey, SubscriptionKeyHash>>
        subscriptions_;
    mutable std::mutex subscriptions_mutex_;

    void add_subscription(const std::string& address,
                          const std::string& service_uuid,
                          const std::string& characteristic_uuid);

    void remove_subscription(const std::string& address,
                             const std::string& service_uuid,
                             const std::string& characteristic_uuid);

    bool has_subscription(const std::string& address,
                          const std::string& service_uuid,
                          const std::string& characteristic_uuid);

  private:
    BleState() = default;
    ~BleState() = default;
    BleState(const BleState&) = delete;
    BleState& operator=(const BleState&) = delete;

    std::unique_ptr<SimpleCommKit::SimpleCommKitBleCentral> ble_;
    bool initialized_ = false;
    std::mutex mutex_;

    /// address → buffered notification entries
    std::unordered_map<std::string, std::vector<NotificationEntry>> notification_buffer_;
    std::mutex notification_buffer_mutex_;
};

/// Convert a hex string (e.g. "00FF" or "01 02 AA") to a byte vector.
/// Throws std::runtime_error on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

/// Convert a byte vector to a hex string (lowercase, no spaces).
std::string bytes_to_hex(const std::vector<uint8_t>& data);

/// Try to interpret binary data as UTF-8; invalid bytes are skipped.
std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data);

/// Convert a BlePeripheral struct to a JSON object.
Json peripheral_to_json(const SimpleCommKit::SimpleCommKitBlePeripheral& p);

/// Convert a BleAdapter struct to a JSON object.
Json adapter_to_json(const SimpleCommKit::SimpleCommKitBleAdapter& a);

/// Convert a BleService struct to a JSON object.
Json service_to_json(const SimpleCommKit::SimpleCommKitBleService& s);

/// Convert a BleCharacteristic struct to a JSON object.
Json characteristic_to_json(const SimpleCommKit::SimpleCommKitBleCharacteristic& c);

/// Convert manufacturer data map to a JSON object (key = hex string).
Json manufacturer_to_json(const std::map<uint16_t, std::vector<uint8_t>>& mfr);

/// Convert address_type enum to string.
const char* address_type_name(int32_t type);

} // namespace SimpleCommKitAiBleFastmcpp
