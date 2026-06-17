#include "mqtt_state.hpp"

#include <SimpleCommKitErrorMap.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace SimpleCommKitAiMqttClientFastmcpp
{

// ===========================================================================
// MqttState singleton
// ===========================================================================

MqttState& MqttState::instance()
{
    static MqttState s;
    return s;
}

bool MqttState::init()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_)
        return mqtt_ != nullptr;

    try
    {
        mqtt_ = std::make_unique<SimpleCommKit::SimpleCommKitMqttClient>();

        // Register callbacks
        mqtt_->setCallback_OnConnected([this]() {
            connected_ = true;
            std::cerr << "[SimpleCommKitAiMqttClientFastmcpp] MQTT connected to "
                      << current_host_ << ":" << current_port_ << std::endl;
        });

        mqtt_->setCallback_OnDisconnected([this]() {
            connected_ = false;
            std::cerr << "[SimpleCommKitAiMqttClientFastmcpp] MQTT disconnected"
                      << std::endl;
        });

        mqtt_->setCallback_OnMessage(
            [this](const std::string& topic, const std::vector<uint8_t>& payload) {
                push_message(topic, payload);
                std::cerr << "[SimpleCommKitAiMqttClientFastmcpp] Message on '"
                          << topic << "' (" << payload.size() << " bytes)"
                          << std::endl;
            });

        mqtt_->setCallback_OnError([this](SimpleCommKit::ErrorCode code) {
            std::string desc =
                SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(code);
            std::cerr << "[SimpleCommKitAiMqttClientFastmcpp] Error " << code
                      << ": " << desc << std::endl;
        });

        initialized_ = true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[SimpleCommKitAiMqttClientFastmcpp] MQTT init failed: "
                  << e.what() << std::endl;
        initialized_ = true; // mark as attempted
    }

    return mqtt_ != nullptr;
}

SimpleCommKit::SimpleCommKitMqttClient& MqttState::ensure_mqtt()
{
    if (!init())
        throw std::runtime_error("MQTT client not available");
    return *mqtt_;
}

// ===========================================================================
// Message buffer
// ===========================================================================

void MqttState::push_message(const std::string& topic,
                              const std::vector<uint8_t>& data)
{
    MqttMessageEntry entry;
    entry.topic = topic;
    entry.data_hex = bytes_to_hex(data);
    entry.data_utf8 = bytes_to_utf8_safe(data);
    entry.data_length = data.size();

    std::lock_guard<std::mutex> lock(message_buffer_mutex_);
    message_buffer_[topic].push_back(std::move(entry));
}

std::vector<MqttMessageEntry> MqttState::drain_messages(const std::string& topic)
{
    std::lock_guard<std::mutex> lock(message_buffer_mutex_);

    std::vector<MqttMessageEntry> result;

    if (topic.empty())
    {
        // Drain all topics
        for (auto& [t, entries] : message_buffer_)
        {
            for (auto& e : entries)
                result.push_back(std::move(e));
        }
        message_buffer_.clear();
    }
    else
    {
        auto it = message_buffer_.find(topic);
        if (it != message_buffer_.end())
        {
            result = std::move(it->second);
            it->second.clear();
        }
    }

    return result;
}

void MqttState::clear_message_buffers()
{
    std::lock_guard<std::mutex> lock(message_buffer_mutex_);
    message_buffer_.clear();
}

// ===========================================================================
// Cleanup
// ===========================================================================

void MqttState::cleanup()
{
    if (!initialized_ || !mqtt_)
        return;

    std::cerr << "[SimpleCommKitAiMqttClientFastmcpp] Cleaning up MQTT..."
              << std::endl;

    try
    {
        if (connected_)
            mqtt_->disconnect();
    }
    catch (...)
    {
        // Ignore disconnect errors during cleanup
    }

    connected_ = false;
    clear_message_buffers();

    std::cerr << "[SimpleCommKitAiMqttClientFastmcpp] MQTT cleanup complete."
              << std::endl;
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
        throw std::runtime_error(
            "Hex string must have an even number of characters");

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

        bytes.push_back(
            static_cast<uint8_t>((hex_val(high) << 4) | hex_val(low)));
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

} // namespace SimpleCommKitAiMqttClientFastmcpp
