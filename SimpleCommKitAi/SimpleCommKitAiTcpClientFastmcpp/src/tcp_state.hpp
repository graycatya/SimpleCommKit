#pragma once

#include <SimpleCommKitTcp.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SimpleCommKitAiTcpClientFastmcpp
{

/// Buffered message entry from the TCP client's OnMessage callback.
struct TcpMessageEntry
{
    std::string data_hex;
    std::string data_utf8;
    size_t      data_length = 0;
};

/// Singleton that holds TCP client state shared across all MCP tools.
/// Mirrors the Python `TcpClientState` class in SimpleCommKitAiTcpClient.
class TcpClientState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static TcpClientState& instance();

    /// Lazy-init the underlying SimpleCommKitTcpClient and install callbacks.
    /// Safe to call multiple times (idempotent).
    void ensure_client();

    /// Check whether the client has been created (even if not connected).
    bool has_client();

    /// Get the raw TCP client pointer; returns nullptr if not created.
    SimpleCommKit::SimpleCommKitTcpClient* get_client();

    /// Destroy the current client instance.
    void destroy_client();

    // -----------------------------------------------------------------------
    // Message buffer access (thread-safe)
    // -----------------------------------------------------------------------

    /// Retrieve and clear all buffered messages.
    std::vector<TcpMessageEntry> drain_messages();

    /// Clear all buffered messages without returning them.
    void clear_messages();

  private:
    TcpClientState()  = default;
    ~TcpClientState() = default;
    TcpClientState(const TcpClientState&) = delete;
    TcpClientState& operator=(const TcpClientState&) = delete;

    std::mutex client_mutex_;
    std::unique_ptr<SimpleCommKit::SimpleCommKitTcpClient> client_;
    bool callbacks_installed_ = false;

    std::mutex message_mutex_;
    std::vector<TcpMessageEntry> message_buffer_;

    /// Install OnConnected / OnDisconnected / OnMessage / OnError callbacks.
    void install_callbacks();
};

/// Convert a hex string (e.g. "00FF" or "01 02 AA") to a byte vector.
/// Throws std::runtime_error on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

/// Convert a byte vector to a hex string (no spaces).
std::string bytes_to_hex(const std::vector<uint8_t>& data);

/// Try to interpret binary data as UTF-8; invalid bytes are skipped.
std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data);

} // namespace SimpleCommKitAiTcpClientFastmcpp
