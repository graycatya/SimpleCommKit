#include "tcp_server_state.hpp"

#include <SimpleCommKitErrorMap.hpp>

#include <cctype>
#include <iostream>
#include <stdexcept>

namespace SimpleCommKitAiTcpServerFastmcpp
{

// ===========================================================================
// TcpServerState singleton
// ===========================================================================

TcpServerState& TcpServerState::instance()
{
    static TcpServerState s;
    return s;
}

void TcpServerState::ensure_server()
{
    std::lock_guard<std::mutex> lock(server_mutex_);

    if (server_)
        return;

    server_ = std::make_unique<SimpleCommKit::SimpleCommKitTcpServer>();
    install_callbacks();
}

bool TcpServerState::has_server()
{
    std::lock_guard<std::mutex> lock(server_mutex_);
    return server_ != nullptr;
}

SimpleCommKit::SimpleCommKitTcpServer* TcpServerState::get_server()
{
    std::lock_guard<std::mutex> lock(server_mutex_);
    return server_.get();
}

void TcpServerState::destroy_server()
{
    std::lock_guard<std::mutex> lock(server_mutex_);
    if (server_ && server_->isRunning())
        server_->stop();
    server_.reset();
    callbacks_installed_ = false;

    // Also clear clients and messages
    {
        std::lock_guard<std::mutex> sl(state_mutex_);
        connected_clients_.clear();
        message_buffer_.clear();
    }
}

void TcpServerState::install_callbacks()
{
    if (callbacks_installed_ || !server_)
        return;

    server_->setCallback_OnClientConnected(
        [this](uint32_t client_id)
        {
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                connected_clients_[client_id] = "client_" + std::to_string(client_id);
            }
            std::cerr << "[SimpleCommKitAiTcpServerFastmcpp] Client connected: "
                      << client_id << std::endl;
        });

    server_->setCallback_OnClientDisconnected(
        [this](uint32_t client_id)
        {
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                connected_clients_.erase(client_id);
            }
            std::cerr << "[SimpleCommKitAiTcpServerFastmcpp] Client disconnected: "
                      << client_id << std::endl;
        });

    server_->setCallback_OnMessage(
        [this](uint32_t client_id, const std::vector<uint8_t>& data)
        {
            TcpServerMessageEntry entry;
            entry.client_id   = client_id;
            entry.data_hex    = bytes_to_hex(data);
            entry.data_utf8   = bytes_to_utf8_safe(data);
            entry.data_length = data.size();

            std::lock_guard<std::mutex> lock(state_mutex_);
            message_buffer_.push_back(std::move(entry));
        });

    server_->setCallback_OnError(
        [](SimpleCommKit::ErrorCode code)
        {
            std::string desc =
                SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(code);
            std::cerr << "[SimpleCommKitAiTcpServerFastmcpp] Error: "
                      << code << " - " << desc << std::endl;
        });

    callbacks_installed_ = true;
}

void TcpServerState::add_client(uint32_t client_id)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    connected_clients_[client_id] = "client_" + std::to_string(client_id);
}

void TcpServerState::remove_client(uint32_t client_id)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    connected_clients_.erase(client_id);
}

std::vector<TcpServerMessageEntry> TcpServerState::drain_messages()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto drained = std::move(message_buffer_);
    message_buffer_.clear();
    return drained;
}

void TcpServerState::clear_messages()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    message_buffer_.clear();
}

// ===========================================================================
// Utility functions
// ===========================================================================

std::vector<uint8_t> hex_to_bytes(const std::string& hex)
{
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
        char low  = cleaned[i + 1];

        auto hex_val = [](char c) -> int
        {
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
        if ((b & 0xE0) == 0xC0 && i + 1 < data.size() && (data[i + 1] & 0xC0) == 0x80)
        {
            result.push_back(static_cast<char>(b));
            result.push_back(static_cast<char>(data[i + 1]));
            i += 2;
            continue;
        }

        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        if ((b & 0xF0) == 0xE0 && i + 2 < data.size() &&
            (data[i + 1] & 0xC0) == 0x80 && (data[i + 2] & 0xC0) == 0x80)
        {
            result.push_back(static_cast<char>(b));
            result.push_back(static_cast<char>(data[i + 1]));
            result.push_back(static_cast<char>(data[i + 2]));
            i += 3;
            continue;
        }

        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        if ((b & 0xF8) == 0xF0 && i + 3 < data.size() &&
            (data[i + 1] & 0xC0) == 0x80 && (data[i + 2] & 0xC0) == 0x80 &&
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

} // namespace SimpleCommKitAiTcpServerFastmcpp
