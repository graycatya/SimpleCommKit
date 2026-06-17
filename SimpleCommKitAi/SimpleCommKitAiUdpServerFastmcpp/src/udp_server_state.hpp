#pragma once

#include <SimpleCommKitUdp.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SimpleCommKitAiUdpServerFastmcpp
{

/// Buffered message entry from the UDP server's OnMessage callback.
/// Includes sender address since UDP is connectionless.
struct UdpServerMessageEntry
{
    std::string from_host;
    int         from_port   = 0;
    std::string data_hex;
    std::string data_utf8;
    size_t      data_length = 0;
};

/// Singleton that holds UDP server state shared across all MCP tools.
/// Mirrors the Python `UdpServerState` class in SimpleCommKitAiUdpServer.
class UdpServerState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static UdpServerState& instance();

    /// Lazy-init the underlying SimpleCommKitUdpServer and install callbacks.
    /// Safe to call multiple times (idempotent).
    void ensure_server();

    /// Check whether the server has been created (even if not running).
    bool has_server();

    /// Get the raw UDP server pointer; returns nullptr if not created.
    SimpleCommKit::SimpleCommKitUdpServer* get_server();

    /// Destroy the current server instance.
    void destroy_server();

    // -----------------------------------------------------------------------
    // Message buffer access (thread-safe)
    // -----------------------------------------------------------------------

    /// Retrieve and clear all buffered messages.
    std::vector<UdpServerMessageEntry> drain_messages();

    /// Clear all buffered messages without returning them.
    void clear_messages();

  private:
    UdpServerState()  = default;
    ~UdpServerState() = default;
    UdpServerState(const UdpServerState&) = delete;
    UdpServerState& operator=(const UdpServerState&) = delete;

    std::mutex server_mutex_;
    std::unique_ptr<SimpleCommKit::SimpleCommKitUdpServer> server_;
    bool callbacks_installed_ = false;

    std::mutex message_mutex_;
    std::vector<UdpServerMessageEntry> message_buffer_;

    /// Install OnMessage / OnError callbacks.
    void install_callbacks();
};

/// Convert a hex string (e.g. "00FF" or "01 02 AA") to a byte vector.
/// Throws std::runtime_error on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

/// Convert a byte vector to a hex string (no spaces).
std::string bytes_to_hex(const std::vector<uint8_t>& data);

/// Try to interpret binary data as UTF-8; invalid bytes are skipped.
std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data);

} // namespace SimpleCommKitAiUdpServerFastmcpp
