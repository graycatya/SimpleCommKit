#pragma once

#include <SimpleCommKitWebSocket.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SimpleCommKitAiWebSocketClientFastmcpp
{

/// Buffered message entry from the WebSocket client's OnMessage callback.
struct WsMessageEntry
{
    std::string data_hex;
    std::string data_utf8;
    size_t      data_length = 0;
};

/// Singleton that holds WebSocket client state shared across all MCP tools.
/// Mirrors the Python WebSocketClientState class in SimpleCommKitAiWebSocketClient.
class WsClientState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static WsClientState& instance();

    /// Lazy-init the underlying SimpleCommKitWebSocketClient and install callbacks.
    /// Safe to call multiple times (idempotent).
    void ensure_client();

    /// Check whether the client has been created (even if not connected).
    bool has_client();

    /// Get the raw WebSocket client pointer; returns nullptr if not created.
    SimpleCommKit::SimpleCommKitWebSocketClient* get_client();

    /// Destroy the current client instance.
    void destroy_client();

    // -----------------------------------------------------------------------
    // Message buffer access (thread-safe)
    // -----------------------------------------------------------------------

    /// Retrieve and clear all buffered messages.
    std::vector<WsMessageEntry> drain_messages();

    /// Clear all buffered messages without returning them.
    void clear_messages();

  private:
    WsClientState()  = default;
    ~WsClientState() = default;
    WsClientState(const WsClientState&) = delete;
    WsClientState& operator=(const WsClientState&) = delete;

    std::mutex client_mutex_;
    std::unique_ptr<SimpleCommKit::SimpleCommKitWebSocketClient> client_;
    bool callbacks_installed_ = false;

    std::mutex message_mutex_;
    std::vector<WsMessageEntry> message_buffer_;

    /// Install OnOpen / OnClose / OnMessage / OnError callbacks.
    void install_callbacks();
};

/// Convert a hex string (e.g. "00FF" or "01 02 AA") to a byte vector.
/// Throws std::runtime_error on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

/// Convert a byte vector to a hex string (no spaces).
std::string bytes_to_hex(const std::vector<uint8_t>& data);

/// Try to interpret binary data as UTF-8; invalid bytes are skipped.
std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data);

} // namespace SimpleCommKitAiWebSocketClientFastmcpp
