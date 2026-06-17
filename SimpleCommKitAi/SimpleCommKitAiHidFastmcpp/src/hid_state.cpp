#include "hid_state.hpp"

#include <SimpleCommKitErrorMap.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace SimpleCommKitAiHidFastmcpp
{

// ===========================================================================
// HidState singleton
// ===========================================================================

HidState& HidState::instance()
{
    static HidState s;
    return s;
}

bool HidState::init()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_)
        return hid_ != nullptr;

    try
    {
        hid_ = std::make_unique<SimpleCommKit::SimpleCommKitHid>();

        // Register error callback
        hid_->set_Callback_Error([](SimpleCommKit::ErrorCode code)
                                 {
            std::string desc =
                SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(code);
            std::cerr << "[SimpleCommKitAiHidFastmcpp] Error " << code << ": " << desc
                      << std::endl; });

        initialized_ = true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[SimpleCommKitAiHidFastmcpp] HID init failed: " << e.what() << std::endl;
        initialized_ = true;
    }

    return hid_ != nullptr;
}

SimpleCommKit::SimpleCommKitHid& HidState::ensure_hid()
{
    if (!init())
        throw std::runtime_error("HID hardware not available");
    return *hid_;
}

void HidState::setup_read_callback()
{
    auto& hid = *hid_;
    hid.set_Callback_On_Read(
        [this](const SimpleCommKit::SimpleCommKitHidDeviceInfo& device_info,
               const std::vector<uint8_t>& data)
        {
            std::string path = device_info.path;
            ReadDataEntry entry;
            entry.path = path;
            entry.data_hex = bytes_to_hex(data);
            entry.data_utf8 = bytes_to_utf8_safe(data);
            entry.data_length = data.size();

            std::lock_guard<std::mutex> lock(read_buffer_mutex_);
            read_buffer_[path].push_back(std::move(entry));
        });
}

std::vector<ReadDataEntry> HidState::drain_read_buffer(const std::string& path)
{
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);
    auto it = read_buffer_.find(path);
    if (it == read_buffer_.end())
        return {};
    auto samples = std::move(it->second);
    it->second.clear();
    return samples;
}

void HidState::remove_read_buffer(const std::string& path)
{
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);
    read_buffer_.erase(path);
}

void HidState::clear_read_buffers()
{
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);
    read_buffer_.clear();
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
        if ((b & 0xF0) == 0xE0 && i + 2 < data.size() && (data[i + 1] & 0xC0) == 0x80 &&
            (data[i + 2] & 0xC0) == 0x80)
        {
            result.push_back(static_cast<char>(b));
            result.push_back(static_cast<char>(data[i + 1]));
            result.push_back(static_cast<char>(data[i + 2]));
            i += 3;
            continue;
        }

        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        if ((b & 0xF8) == 0xF0 && i + 3 < data.size() && (data[i + 1] & 0xC0) == 0x80 &&
            (data[i + 2] & 0xC0) == 0x80 && (data[i + 3] & 0xC0) == 0x80)
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

Json device_info_to_json(const SimpleCommKit::SimpleCommKitHidDeviceInfo& d)
{
    int bus = static_cast<int>(d.bus_type);
    return Json{
        {"path", d.path},
        {"manufacturer_string", d.manufacturer_string},
        {"product_string", d.product_string},
        {"serial_number", d.serial_number},
        {"bus_type", bus},
        {"bus_type_name", bus_type_name(bus)},
        {"interface_number", d.interface_number},
        {"release_number", d.release_number},
    };
}

const char* bus_type_name(int bus_type)
{
    switch (bus_type)
    {
    case 0:
        return "UNKNOWN";
    case 1:
        return "USB";
    case 2:
        return "BLUETOOTH";
    case 3:
        return "I2C";
    case 4:
        return "SPI";
    default:
        return "UNKNOWN";
    }
}

} // namespace SimpleCommKitAiHidFastmcpp
