#pragma once

#include <SimpleCommKitMqttClient.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace SimpleCommKitAiMqttClientFastmcpp
{

using Json = nlohmann::json;

/// Buffered MQTT message entry stored per topic.
struct MqttMessageEntry
{
    std::string topic;
    std::string data_hex;
    std::string data_utf8;
    size_t data_length = 0;
};

/// Singleton that holds MQTT client state shared across all MCP tools.
/// Mirrors the Python `MqttState` class in SimpleCommKitAiMqttClient.
class MqttState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static MqttState& instance();

    /// Lazy-initialize the MQTT subsystem. Safe to call multiple times.
    /// @return true if the MQTT client is ready.
    bool init();

    /// Ensure MQTT client is initialized; throws std::runtime_error on failure.
    SimpleCommKit::SimpleCommKitMqttClient& ensure_mqtt();

    // -----------------------------------------------------------------------
    // Connection state
    // -----------------------------------------------------------------------
    std::atomic<bool> connected_{false};

    // -----------------------------------------------------------------------
    // Message buffer access (thread-safe)
    // -----------------------------------------------------------------------
    void push_message(const std::string& topic,
                      const std::vector<uint8_t>& data);

    /// Retrieve and clear all buffered messages for a topic.
    /// If topic is empty, drain all messages.
    std::vector<MqttMessageEntry> drain_messages(const std::string& topic = "");

    /// Clear all message buffers (called on disconnect / cleanup).
    void clear_message_buffers();

    // -----------------------------------------------------------------------
    // Current connection info
    // -----------------------------------------------------------------------
    std::string current_host_;
    int current_port_ = 1883;
    bool current_ssl_ = false;
    std::string current_client_id_;
    std::string current_username_;
    std::string current_password_;

    // -----------------------------------------------------------------------
    // Shutdown
    // -----------------------------------------------------------------------
    /// Disconnect and release MQTT resources.
    void cleanup();

  private:
    MqttState() = default;
    ~MqttState() = default;
    MqttState(const MqttState&) = delete;
    MqttState& operator=(const MqttState&) = delete;

    std::unique_ptr<SimpleCommKit::SimpleCommKitMqttClient> mqtt_;
    bool initialized_ = false;
    std::mutex mutex_;

    /// topic → buffered message entries
    std::unordered_map<std::string, std::vector<MqttMessageEntry>> message_buffer_;
    std::mutex message_buffer_mutex_;
};

// ===========================================================================
// Utility functions
// ===========================================================================

/// Convert a hex string (e.g. "00FF" or "01 02 AA") to a byte vector.
/// Throws std::runtime_error on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

/// Convert a byte vector to a hex string (lowercase, no spaces).
std::string bytes_to_hex(const std::vector<uint8_t>& data);

/// Try to interpret binary data as UTF-8; invalid bytes are skipped.
std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data);

} // namespace SimpleCommKitAiMqttClientFastmcpp
