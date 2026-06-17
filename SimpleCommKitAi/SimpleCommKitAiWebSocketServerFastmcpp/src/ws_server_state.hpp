#pragma once

#include <SimpleCommKitWebSocket.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace SimpleCommKitAiWebSocketServerFastmcpp
{

/// Buffered message entry from the WebSocket server's OnMessage callback.
struct WsServerMessageEntry
{
    uint32_t    client_id = 0;
    std::string data_hex;
    std::string data_utf8;
    size_t      data_length = 0;
};

/// Singleton that holds WebSocket server state shared across all MCP tools.
/// Mirrors the Python WebSocketServerState class in SimpleCommKitAiWebSocketServer.
class WsServerState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static WsServerState& instance();

    /// Lazy-init the underlying SimpleCommKitWebSocketServer and install callbacks.
    /// Safe to call multiple times (idempotent).
    void ensure_server();

    /// Check whether the server has been created.
    bool has_server();

    /// Get the raw WebSocket server pointer; returns nullptr if not created.
    SimpleCommKit::SimpleCommKitWebSocketServer* get_server();

    /// Destroy the current server instance.
    void destroy_server();

    // -----------------------------------------------------------------------
    // Connected client tracking (thread-safe)
    // -----------------------------------------------------------------------

    /// Record a newly connected client.
    void add_client(uint32_t client_id);

    /// Remove a disconnected client.
    void remove_client(uint32_t client_id);

    /// Get the set of connected client IDs.
    std::unordered_set<uint32_t> get_clients();

    // -----------------------------------------------------------------------
    // Message buffer access (thread-safe)
    // -----------------------------------------------------------------------

    /// Retrieve and clear all buffered messages.
    std::vector<WsServerMessageEntry> drain_messages();

    /// Clear all buffered messages without returning them.
    void clear_messages();

  private:
    WsServerState()  = default;
    ~WsServerState() = default;
    WsServerState(const WsServerState&) = delete;
    WsServerState& operator=(const WsServerState&) = delete;

    std::mutex server_mutex_;
    std::unique_ptr<SimpleCommKit::SimpleCommKitWebSocketServer> server_;
    bool callbacks_installed_ = false;

    std::mutex state_mutex_;
    std::unordered_set<uint32_t> connected_clients_;

    std::mutex message_mutex_;
    std::vector<WsServerMessageEntry> message_buffer_;

    /// Install OnClientConnected / OnClientDisconnected / OnMessage / OnError callbacks.
    void install_callbacks();
};

/// Convert a hex string (e.g. "00FF" or "01 02 AA") to a byte vector.
/// Throws std::runtime_error on invalid input.
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

/// Convert a byte vector to a hex string (no spaces).
std::string bytes_to_hex(const std::vector<uint8_t>& data);

/// Try to interpret binary data as UTF-8; invalid bytes are skipped.
std::string bytes_to_utf8_safe(const std::vector<uint8_t>& data);

} // namespace SimpleCommKitAiWebSocketServerFastmcpp
