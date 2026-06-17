#include "ble_state.hpp"

#include <SimpleCommKitErrorMap.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace SimpleCommKitAiBleFastmcpp
{

// ===========================================================================
// BleState singleton
// ===========================================================================

BleState& BleState::instance()
{
    static BleState s;
    return s;
}

bool BleState::init()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_)
        return ble_ != nullptr;

    try
    {
        ble_ = std::make_unique<SimpleCommKit::SimpleCommKitBleCentral>();

        // Register error callback (safe to set any time)
        ble_->set_Callback_Error([](SimpleCommKit::ErrorCode code) {
            std::string desc =
                SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(code);
            std::cerr << "[SimpleCommKitAiBleFastmcpp] Error " << code << ": " << desc
                      << std::endl;
        });

        initialized_ = true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[SimpleCommKitAiBleFastmcpp] BLE init failed: " << e.what() << std::endl;
        initialized_ = true; // mark as attempted
    }

    return ble_ != nullptr;
}

SimpleCommKit::SimpleCommKitBleCentral& BleState::ensure_ble()
{
    if (!init())
        throw std::runtime_error("BLE hardware not available");
    return *ble_;
}

// ===========================================================================
// Scan results
// ===========================================================================

void BleState::add_scan_result(const SimpleCommKit::SimpleCommKitBlePeripheral& p)
{
    std::lock_guard<std::mutex> lock(scan_results_mutex_);
    // Replace by address if already present, otherwise append
    for (auto& existing : scan_results)
    {
        if (existing.address == p.address)
        {
            existing = p; // update RSSI etc.
            return;
        }
    }
    scan_results.push_back(p);
}

void BleState::clear_scan_results()
{
    std::lock_guard<std::mutex> lock(scan_results_mutex_);
    scan_results.clear();
}

// ===========================================================================
// Connected peripherals
// ===========================================================================

void BleState::set_connected(const std::string& address, bool connected)
{
    std::lock_guard<std::mutex> lock(peripherals_mutex_);
    peripherals_[address].connected = connected;
}

bool BleState::is_connected(const std::string& address)
{
    std::lock_guard<std::mutex> lock(peripherals_mutex_);
    auto it = peripherals_.find(address);
    return it != peripherals_.end() && it->second.connected;
}

// ===========================================================================
// Notifications
// ===========================================================================

void BleState::push_notification(const std::string& address,
                                  const std::string& service_uuid,
                                  const std::string& characteristic_uuid,
                                  const std::vector<uint8_t>& data)
{
    NotificationEntry entry;
    entry.address = address;
    entry.service_uuid = service_uuid;
    entry.characteristic_uuid = characteristic_uuid;
    entry.data_hex = bytes_to_hex(data);
    entry.data_utf8 = bytes_to_utf8_safe(data);
    entry.data_length = data.size();

    std::lock_guard<std::mutex> lock(notification_buffer_mutex_);
    notification_buffer_[address].push_back(std::move(entry));
}

std::vector<NotificationEntry> BleState::drain_notifications(const std::string& address)
{
    std::lock_guard<std::mutex> lock(notification_buffer_mutex_);
    auto it = notification_buffer_.find(address);
    if (it == notification_buffer_.end())
        return {};
    auto entries = std::move(it->second);
    it->second.clear();
    return entries;
}

void BleState::remove_notification_buffer(const std::string& address)
{
    std::lock_guard<std::mutex> lock(notification_buffer_mutex_);
    notification_buffer_.erase(address);
}

void BleState::clear_notification_buffers()
{
    std::lock_guard<std::mutex> lock(notification_buffer_mutex_);
    notification_buffer_.clear();
}

// ===========================================================================
// Subscriptions
// ===========================================================================

void BleState::add_subscription(const std::string& address,
                                 const std::string& service_uuid,
                                 const std::string& characteristic_uuid)
{
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_[address].insert({service_uuid, characteristic_uuid});
}

void BleState::remove_subscription(const std::string& address,
                                    const std::string& service_uuid,
                                    const std::string& characteristic_uuid)
{
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = subscriptions_.find(address);
    if (it != subscriptions_.end())
    {
        it->second.erase({service_uuid, characteristic_uuid});
        if (it->second.empty())
            subscriptions_.erase(it);
    }
}

bool BleState::has_subscription(const std::string& address,
                                 const std::string& service_uuid,
                                 const std::string& characteristic_uuid)
{
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = subscriptions_.find(address);
    if (it == subscriptions_.end())
        return false;
    return it->second.count({service_uuid, characteristic_uuid}) > 0;
}

// ===========================================================================
// Cleanup (called on shutdown to avoid blocking)
// ===========================================================================

void BleState::cleanup()
{
    if (!initialized_ || !ble_)
        return;

    std::cerr << "[SimpleCommKitAiBleFastmcpp] Cleaning up BLE connections..." << std::endl;

    // 1. Collect connected addresses and their subscriptions under the lock
    std::vector<std::pair<std::string, std::vector<SubscriptionKey>>> to_disconnect;
    {
        std::lock_guard<std::mutex> lock(peripherals_mutex_);
        std::lock_guard<std::mutex> slock(subscriptions_mutex_);

        // Make a snapshot of peripherals and subscriptions before cleanup
        for (auto& [address, info] : peripherals_)
        {
            std::vector<SubscriptionKey> subs;
            auto sit = subscriptions_.find(address);
            if (sit != subscriptions_.end())
                subs.assign(sit->second.begin(), sit->second.end());
            to_disconnect.emplace_back(address, std::move(subs));
        }
    }

    // 2. Unsubscribe then disconnect each device
    for (auto& [address, subs] : to_disconnect)
    {
        try
        {
            // Need to set current peripheral before operating
            {
                std::lock_guard<std::mutex> lock(peripherals_mutex_);
                auto it = peripherals_.find(address);
                if (it != peripherals_.end())
                    ble_->set_CurrentPeripheral(it->second.peripheral);
            }

            // Unsubscribe from all notifications/indications first
            for (auto& sub : subs)
            {
                try
                {
                    ble_->peripheral_Unsubscribe(sub.service_uuid, sub.characteristic_uuid);
                }
                catch (...)
                {
                    // Ignore unsubscribe errors during cleanup
                }
            }

            // Disconnect the peripheral
            try
            {
                ble_->peripheral_Disconnect();
            }
            catch (...)
            {
                // Ignore disconnect errors during cleanup
            }

            std::cerr << "[SimpleCommKitAiBleFastmcpp] Disconnected: " << address << std::endl;
        }
        catch (...)
        {
            // Ignore errors setting current peripheral
        }
    }

    // 3. Clear all tracked state
    {
        std::lock_guard<std::mutex> lock(peripherals_mutex_);
        peripherals_.clear();
    }
    clear_scan_results();
    clear_notification_buffers();
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_.clear();
    }

    // 4. Try to power off the adapter (only if we previously cached one)
    if (current_adapter_.has_value())
    {
        try
        {
            ble_->set_CurrentAdapter(*current_adapter_);
            if (ble_->adapter_Is_Powered())
                ble_->adapter_Power_Off();
        }
        catch (...)
        {
            // Ignore power-off errors
        }
    }

    std::cerr << "[SimpleCommKitAiBleFastmcpp] BLE cleanup complete." << std::endl;
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
        char low = cleaned[i + 1];

        auto hex_val = [](char c) -> int {
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
        if ((b & 0xE0) == 0xC0 && i + 1 < data.size() &&
            (data[i + 1] & 0xC0) == 0x80)
        {
            result.push_back(static_cast<char>(b));
            result.push_back(static_cast<char>(data[i + 1]));
            i += 2;
            continue;
        }

        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        if ((b & 0xF0) == 0xE0 && i + 2 < data.size() &&
            (data[i + 1] & 0xC0) == 0x80 &&
            (data[i + 2] & 0xC0) == 0x80)
        {
            result.push_back(static_cast<char>(b));
            result.push_back(static_cast<char>(data[i + 1]));
            result.push_back(static_cast<char>(data[i + 2]));
            i += 3;
            continue;
        }

        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        if ((b & 0xF8) == 0xF0 && i + 3 < data.size() &&
            (data[i + 1] & 0xC0) == 0x80 &&
            (data[i + 2] & 0xC0) == 0x80 &&
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

Json peripheral_to_json(const SimpleCommKit::SimpleCommKitBlePeripheral& p)
{
    return Json{
        {"identifier", p.identifier},
        {"address", p.address},
        {"address_type", static_cast<int32_t>(p.address_type)},
        {"address_type_name", address_type_name(static_cast<int32_t>(p.address_type))},
        {"rssi", p.rssi},
        {"manufacturer_data", manufacturer_to_json(p.manufacturer)},
    };
}

Json adapter_to_json(const SimpleCommKit::SimpleCommKitBleAdapter& a)
{
    return Json{
        {"dev_identifier", a.dev_identifier},
        {"dev_address", a.dev_address},
    };
}

Json service_to_json(const SimpleCommKit::SimpleCommKitBleService& s)
{
    Json chars = Json::array();
    for (const auto& c : s.characteristics)
        chars.push_back(characteristic_to_json(c));

    return Json{
        {"uuid", s.uuid},
        {"data_hex", bytes_to_hex(s.data)},
        {"characteristics", chars},
    };
}

Json characteristic_to_json(const SimpleCommKit::SimpleCommKitBleCharacteristic& c)
{
    return Json{
        {"uuid", c.uuid},
        {"descriptors_uuid", c.descriptors_uuid},
        {"capabilities", c.capabilities},
        {"can_read", c.can_read},
        {"can_write_request", c.can_write_request},
        {"can_write_command", c.can_write_command},
        {"can_notify", c.can_notify},
        {"can_indicate", c.can_indicate},
    };
}

Json manufacturer_to_json(const std::map<uint16_t, std::vector<uint8_t>>& mfr)
{
    Json result = Json::object();
    for (const auto& kv : mfr)
    {
        std::ostringstream oss;
        oss << std::hex << std::setw(4) << std::setfill('0') << kv.first;
        result[oss.str()] = bytes_to_hex(kv.second);
    }
    return result;
}

const char* address_type_name(int32_t type)
{
    switch (type)
    {
    case 0:
        return "PUBLIC";
    case 1:
        return "RANDOM";
    case 2:
        return "UNSPECIFIED";
    default:
        return "UNKNOWN";
    }
}

} // namespace SimpleCommKitAiBleFastmcpp
