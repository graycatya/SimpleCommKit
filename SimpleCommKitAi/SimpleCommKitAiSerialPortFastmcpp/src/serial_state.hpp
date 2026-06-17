#pragma once

#include <SimpleCommKitSerialPort.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace SimpleCommKitAiSerialPortFastmcpp
{

using Json = nlohmann::json;

/// Read-data entry buffered by the background read thread.
struct SerialReadEntry
{
    std::string port_name;
    std::string data_hex;
    std::string data_utf8;
    size_t data_length = 0;
};

/// Singleton that holds serial-port state shared across all MCP tools.
/// Mirrors the Python `SerialState` class in SimpleCommKitAiSerialPort.
class SerialState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static SerialState& instance();

    /// Create or return the SimpleCommKitSerialPort for the given port name.
    /// The instance is created on first call and reused thereafter.
    SimpleCommKit::SimpleCommKitSerialPort* get_or_create_port(const std::string& port_name);

    /// Check whether a port with the given name is currently managed.
    bool has_port(const std::string& port_name);

    /// Get a managed port; returns nullptr if not found.
    SimpleCommKit::SimpleCommKitSerialPort* get_port(const std::string& port_name);

    /// Remove a managed port from the map and its read buffer.
    void remove_port(const std::string& port_name);

    /// Get a list of all currently managed port names.
    std::vector<std::string> get_port_names();

    /// Remove all managed ports and clear all read buffers.
    void remove_all_ports();

    /// Install the On_Read callback for the given port. Safe to call multiple
    /// times (idempotent).
    void setup_read_callback(const std::string& port_name);

    // -----------------------------------------------------------------------
    // Read buffer access (thread-safe)
    // -----------------------------------------------------------------------

    /// Retrieve and clear all buffered read entries for a port name.
    std::vector<SerialReadEntry> drain_read_buffer(const std::string& port_name);

    /// Remove a port's read buffer (called on close).
    void remove_read_buffer(const std::string& port_name);

    /// Clear all read buffers (called on close-all).
    void clear_read_buffers();

    // -----------------------------------------------------------------------
    // Hotplug
    // -----------------------------------------------------------------------

    std::atomic<bool> hotplug_active{false};

  private:
    SerialState() = default;
    ~SerialState() = default;
    SerialState(const SerialState&) = delete;
    SerialState& operator=(const SerialState&) = delete;

    std::mutex ports_mutex_;
    /// portName -> SimpleCommKitSerialPort instance
    std::unordered_map<std::string, std::unique_ptr<SimpleCommKit::SimpleCommKitSerialPort>> ports_;

    /// portName -> buffered read entries
    std::unordered_map<std::string, std::vector<SerialReadEntry>> read_buffer_;
    std::mutex read_buffer_mutex_;
};

/// Convert a hex string (e.g. "00FF" or "01 02 AA") to a byte vector.
/// Throws std::runtime_error on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

/// Convert a byte vector to a hex string (no spaces).
std::string bytes_to_hex(const std::vector<uint8_t>& data);

/// Try to interpret binary data as UTF-8; invalid bytes are skipped.
std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data);

/// Convert a SerialPortInfo struct to a JSON object.
Json port_info_to_json(const SimpleCommKit::SimpleCommKitSerialPortInfo& info);

/// Convert a Parity enum value to a human-readable name.
const char* parity_name(SimpleCommKit::Parity parity);

/// Convert a StopBits enum value to a human-readable name.
const char* stop_bits_name(SimpleCommKit::StopBits stop_bits);

/// Convert a FlowControl enum value to a human-readable name.
const char* flow_control_name(SimpleCommKit::FlowControl flow_control);

/// Parse a parity string (e.g. "none", "odd") to the enum value.
SimpleCommKit::Parity parse_parity(const std::string& s);

/// Parse a stop-bits string (e.g. "one", "two") to the enum value.
SimpleCommKit::StopBits parse_stop_bits(const std::string& s);

/// Parse a flow-control string (e.g. "none", "hardware", "software") to the enum value.
SimpleCommKit::FlowControl parse_flow_control(const std::string& s);

} // namespace SimpleCommKitAiSerialPortFastmcpp
