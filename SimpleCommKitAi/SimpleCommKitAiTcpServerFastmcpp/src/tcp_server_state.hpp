#pragma once

#include <SimpleCommKitTcp.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SimpleCommKitAiTcpServerFastmcpp
{

/// Buffered message entry from the TCP server's OnMessage callback.
struct TcpServerMessageEntry
{
    uint32_t    client_id = 0;
    std::string data_hex;
    std::string data_utf8;
    size_t      data_length = 0;
};

/// Singleton that holds TCP server state shared across all MCP tools.
/// Mirrors the Python `TcpServerState` class in SimpleCommKitAiTcpServer.
class TcpServerState
{
  public:
    /// Get the global singleton instance (thread-safe).
    static TcpServerState& instance();

    /// Lazy-init the underlying SimpleCommKitTcpServer and install callbacks.
    /// Safe to call multiple times (idempotent).
    void ensure_server();

    /// Check whether the server has been created.
    bool has_server();

    /// Get the raw TCP server pointer; returns nullptr if not created.
    SimpleCommKit::SimpleCommKitTcpServer* get_server();

    /// Destroy the current server instance and clear all state.
    void destroy_server();

    // -----------------------------------------------------------------------
    // Connected clients tracking (thread-safe)
    // -----------------------------------------------------------------------

    /// Add a client to the connected clients map.
    void add_client(uint32_t client_id);

    /// Remove a client from the connected clients map.
    void remove_client(uint32_t client_id);

    // -----------------------------------------------------------------------
    // Message buffer access (thread-safe)
    // -----------------------------------------------------------------------

    /// Retrieve and clear all buffered messages.
    std::vector<TcpServerMessageEntry> drain_messages();

    /// Clear all buffered messages without returning them.
    void clear_messages();

  private:
    TcpServerState()  = default;
    ~TcpServerState() = default;
    TcpServerState(const TcpServerState&) = delete;
    TcpServerState& operator=(const TcpServerState&) = delete;

    std::mutex server_mutex_;
    std::unique_ptr<SimpleCommKit::SimpleCommKitTcpServer> server_;
    bool callbacks_installed_ = false;

    std::mutex state_mutex_;
    std::vector<TcpServerMessageEntry> message_buffer_;
    std::map<uint32_t, std::string> connected_clients_;

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

} // namespace SimpleCommKitAiTcpServerFastmcpp
