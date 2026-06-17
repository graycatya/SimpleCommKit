#include "serial_state.hpp"

#include <SimpleCommKitErrorMap.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace SimpleCommKitAiSerialPortFastmcpp
{

// ===========================================================================
// SerialState singleton
// ===========================================================================

SerialState& SerialState::instance()
{
    static SerialState s;
    return s;
}

SimpleCommKit::SimpleCommKitSerialPort* SerialState::get_or_create_port(const std::string& port_name)
{
    std::lock_guard<std::mutex> lock(ports_mutex_);
    auto it = ports_.find(port_name);
    if (it != ports_.end())
        return it->second.get();

    auto port = std::make_unique<SimpleCommKit::SimpleCommKitSerialPort>();
    auto* raw = port.get();
    ports_[port_name] = std::move(port);
    return raw;
}

bool SerialState::has_port(const std::string& port_name)
{
    std::lock_guard<std::mutex> lock(ports_mutex_);
    return ports_.find(port_name) != ports_.end();
}

SimpleCommKit::SimpleCommKitSerialPort* SerialState::get_port(const std::string& port_name)
{
    std::lock_guard<std::mutex> lock(ports_mutex_);
    auto it = ports_.find(port_name);
    if (it == ports_.end())
        return nullptr;
    return it->second.get();
}

void SerialState::remove_port(const std::string& port_name)
{
    {
        std::lock_guard<std::mutex> lock(ports_mutex_);
        ports_.erase(port_name);
    }
    remove_read_buffer(port_name);
}

std::vector<std::string> SerialState::get_port_names()
{
    std::lock_guard<std::mutex> lock(ports_mutex_);
    std::vector<std::string> names;
    names.reserve(ports_.size());
    for (const auto& kv : ports_)
        names.push_back(kv.first);
    return names;
}

void SerialState::remove_all_ports()
{
    {
        std::lock_guard<std::mutex> lock(ports_mutex_);
        for (auto& kv : ports_)
        {
            if (kv.second && kv.second->is_Open())
                kv.second->close();
        }
        ports_.clear();
    }
    clear_read_buffers();
}

void SerialState::setup_read_callback(const std::string& port_name)
{
    auto* port = get_port(port_name);
    if (!port)
        return;

    port->set_Callback_On_Read(
        [this, port_name](const std::vector<uint8_t>& data)
        {
            SerialReadEntry entry;
            entry.port_name = port_name;
            entry.data_hex = bytes_to_hex(data);
            entry.data_utf8 = bytes_to_utf8_safe(data);
            entry.data_length = data.size();

            std::lock_guard<std::mutex> lock(read_buffer_mutex_);
            read_buffer_[port_name].push_back(std::move(entry));
        });

    // Register error callback
    port->set_Callback_Error(
        [port_name](SimpleCommKit::ErrorCode code)
        {
            std::string desc =
                SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(code);
            std::cerr << "[SimpleCommKitAiSerialPortFastmcpp] Error on " << port_name
                      << ": " << code << " - " << desc << std::endl;
        });
}

std::vector<SerialReadEntry> SerialState::drain_read_buffer(const std::string& port_name)
{
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);
    auto it = read_buffer_.find(port_name);
    if (it == read_buffer_.end())
        return {};
    auto samples = std::move(it->second);
    it->second.clear();
    return samples;
}

void SerialState::remove_read_buffer(const std::string& port_name)
{
    std::lock_guard<std::mutex> lock(read_buffer_mutex_);
    read_buffer_.erase(port_name);
}

void SerialState::clear_read_buffers()
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

Json port_info_to_json(const SimpleCommKit::SimpleCommKitSerialPortInfo& info)
{
    return Json{
        {"port_name", info.portName},
        {"description", info.description},
        {"hardware_id", info.hardwareId},
    };
}

const char* parity_name(SimpleCommKit::Parity parity)
{
    switch (parity)
    {
    case SimpleCommKit::ParityNone:  return "none";
    case SimpleCommKit::ParityOdd:   return "odd";
    case SimpleCommKit::ParityEven:  return "even";
    case SimpleCommKit::ParityMark:  return "mark";
    case SimpleCommKit::ParitySpace: return "space";
    default:                         return "unknown";
    }
}

const char* stop_bits_name(SimpleCommKit::StopBits stop_bits)
{
    switch (stop_bits)
    {
    case SimpleCommKit::StopOne:       return "one";
    case SimpleCommKit::StopOneAndHalf: return "one_and_half";
    case SimpleCommKit::StopTwo:       return "two";
    default:                           return "unknown";
    }
}

const char* flow_control_name(SimpleCommKit::FlowControl flow_control)
{
    switch (flow_control)
    {
    case SimpleCommKit::FlowNone:     return "none";
    case SimpleCommKit::FlowHardware: return "hardware";
    case SimpleCommKit::FlowSoftware: return "software";
    default:                          return "unknown";
    }
}

SimpleCommKit::Parity parse_parity(const std::string& s)
{
    if (s == "none")  return SimpleCommKit::ParityNone;
    if (s == "odd")   return SimpleCommKit::ParityOdd;
    if (s == "even")  return SimpleCommKit::ParityEven;
    if (s == "mark")  return SimpleCommKit::ParityMark;
    if (s == "space") return SimpleCommKit::ParitySpace;
    throw std::runtime_error("Unknown parity: " + s);
}

SimpleCommKit::StopBits parse_stop_bits(const std::string& s)
{
    if (s == "one")          return SimpleCommKit::StopOne;
    if (s == "one_and_half") return SimpleCommKit::StopOneAndHalf;
    if (s == "two")          return SimpleCommKit::StopTwo;
    throw std::runtime_error("Unknown stop bits: " + s);
}

SimpleCommKit::FlowControl parse_flow_control(const std::string& s)
{
    if (s == "none")     return SimpleCommKit::FlowNone;
    if (s == "hardware") return SimpleCommKit::FlowHardware;
    if (s == "software") return SimpleCommKit::FlowSoftware;
    throw std::runtime_error("Unknown flow control: " + s);
}

} // namespace SimpleCommKitAiSerialPortFastmcpp
