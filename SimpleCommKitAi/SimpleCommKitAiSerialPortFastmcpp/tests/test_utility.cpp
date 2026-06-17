/// Tests for utility functions: hex_to_bytes, bytes_to_hex, bytes_to_utf8_safe,
/// port_info_to_json, parity_name, stop_bits_name, flow_control_name,
/// parse_parity, parse_stop_bits, parse_flow_control.

#include "../src/serial_state.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace SimpleCommKitAiSerialPortFastmcpp;

static int failures = 0;

#define TEST(name) std::cout << "  " << name << "... "
#define PASS() std::cout << "PASS" << std::endl
#define FAIL(msg)                                      \
    do                                                 \
    {                                                  \
        std::cout << "FAIL: " << msg << std::endl;     \
        ++failures;                                    \
    } while (0)
#define CHECK(cond, msg)       \
    if (!(cond))               \
        FAIL(msg);             \
    else                       \
        std::cout << "PASS" << std::endl

void test_hex_to_bytes()
{
    TEST("hex_to_bytes basic");
    auto result = hex_to_bytes("00FF");
    CHECK(result.size() == 2 && result[0] == 0x00 && result[1] == 0xFF,
          "expected [0x00, 0xFF]");

    TEST("hex_to_bytes with spaces");
    result = hex_to_bytes("01 02 AA");
    CHECK(result.size() == 3 && result[0] == 0x01 && result[1] == 0x02 &&
              result[2] == 0xAA,
          "expected [0x01, 0x02, 0xAA]");

    TEST("hex_to_bytes lowercase");
    result = hex_to_bytes("ff");
    CHECK(result.size() == 1 && result[0] == 0xFF, "expected [0xFF]");

    TEST("hex_to_bytes invalid length");
    bool threw = false;
    try
    {
        hex_to_bytes("abc");
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    CHECK(threw, "should throw for odd-length hex");

    TEST("hex_to_bytes invalid char");
    threw = false;
    try
    {
        hex_to_bytes("zz");
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    CHECK(threw, "should throw for invalid hex char");
}

void test_bytes_to_hex()
{
    TEST("bytes_to_hex empty");
    auto result = bytes_to_hex({});
    CHECK(result.empty(), "expected empty string");

    TEST("bytes_to_hex basic");
    result = bytes_to_hex({0x00, 0xFF, 0xAB});
    CHECK(result == "00ffab", "expected '00ffab', got '" + result + "'");
}

void test_bytes_to_utf8_safe()
{
    TEST("bytes_to_utf8_safe ASCII only");
    auto result = bytes_to_utf8_safe({0x48, 0x65, 0x6C, 0x6C, 0x6F});
    CHECK(result == "Hello", "expected 'Hello', got '" + result + "'");

    TEST("bytes_to_utf8_safe UTF-8 2-byte");
    result = bytes_to_utf8_safe({0xC3, 0xA9}); // é
    CHECK(result == "\xC3\xA9", "expected UTF-8 é");

    TEST("bytes_to_utf8_safe invalid bytes skipped");
    result = bytes_to_utf8_safe({0x48, 0xFF, 0x65}); // middle byte invalid
    CHECK(result == "He", "expected 'He', got '" + result + "'");
}

void test_port_info_to_json()
{
    TEST("port_info_to_json");
    SimpleCommKit::SimpleCommKitSerialPortInfo info;
    info.portName = "COM3";
    info.description = "USB Serial Port";
    info.hardwareId = "FTDI\\VID_0403+PID_6001";

    auto json = port_info_to_json(info);
    CHECK(json["port_name"] == "COM3", "port_name mismatch");
    CHECK(json["description"] == "USB Serial Port", "description mismatch");
    CHECK(json["hardware_id"] == "FTDI\\VID_0403+PID_6001", "hardware_id mismatch");
    std::cout << "  port_info_to_json fields... PASS" << std::endl;
}

void test_parity_name()
{
    TEST("parity_name");
    CHECK(std::string(parity_name(SimpleCommKit::ParityNone)) == "none", "ParityNone");
    CHECK(std::string(parity_name(SimpleCommKit::ParityOdd)) == "odd", "ParityOdd");
    CHECK(std::string(parity_name(SimpleCommKit::ParityEven)) == "even", "ParityEven");
    CHECK(std::string(parity_name(SimpleCommKit::ParityMark)) == "mark", "ParityMark");
    CHECK(std::string(parity_name(SimpleCommKit::ParitySpace)) == "space", "ParitySpace");
    std::cout << "  parity_name all... PASS" << std::endl;
}

void test_stop_bits_name()
{
    TEST("stop_bits_name");
    CHECK(std::string(stop_bits_name(SimpleCommKit::StopOne)) == "one", "StopOne");
    CHECK(std::string(stop_bits_name(SimpleCommKit::StopOneAndHalf)) == "one_and_half", "StopOneAndHalf");
    CHECK(std::string(stop_bits_name(SimpleCommKit::StopTwo)) == "two", "StopTwo");
    std::cout << "  stop_bits_name all... PASS" << std::endl;
}

void test_flow_control_name()
{
    TEST("flow_control_name");
    CHECK(std::string(flow_control_name(SimpleCommKit::FlowNone)) == "none", "FlowNone");
    CHECK(std::string(flow_control_name(SimpleCommKit::FlowHardware)) == "hardware", "FlowHardware");
    CHECK(std::string(flow_control_name(SimpleCommKit::FlowSoftware)) == "software", "FlowSoftware");
    std::cout << "  flow_control_name all... PASS" << std::endl;
}

void test_parse_parity()
{
    TEST("parse_parity");
    CHECK(parse_parity("none") == SimpleCommKit::ParityNone, "none");
    CHECK(parse_parity("odd") == SimpleCommKit::ParityOdd, "odd");
    CHECK(parse_parity("even") == SimpleCommKit::ParityEven, "even");
    CHECK(parse_parity("mark") == SimpleCommKit::ParityMark, "mark");
    CHECK(parse_parity("space") == SimpleCommKit::ParitySpace, "space");
    std::cout << "  parse_parity all... PASS" << std::endl;

    TEST("parse_parity invalid");
    bool threw = false;
    try
    {
        parse_parity("invalid");
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    CHECK(threw, "should throw for invalid parity");
}

void test_parse_stop_bits()
{
    TEST("parse_stop_bits");
    CHECK(parse_stop_bits("one") == SimpleCommKit::StopOne, "one");
    CHECK(parse_stop_bits("one_and_half") == SimpleCommKit::StopOneAndHalf, "one_and_half");
    CHECK(parse_stop_bits("two") == SimpleCommKit::StopTwo, "two");
    std::cout << "  parse_stop_bits all... PASS" << std::endl;

    TEST("parse_stop_bits invalid");
    bool threw = false;
    try
    {
        parse_stop_bits("three");
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    CHECK(threw, "should throw for invalid stop bits");
}

void test_parse_flow_control()
{
    TEST("parse_flow_control");
    CHECK(parse_flow_control("none") == SimpleCommKit::FlowNone, "none");
    CHECK(parse_flow_control("hardware") == SimpleCommKit::FlowHardware, "hardware");
    CHECK(parse_flow_control("software") == SimpleCommKit::FlowSoftware, "software");
    std::cout << "  parse_flow_control all... PASS" << std::endl;

    TEST("parse_flow_control invalid");
    bool threw = false;
    try
    {
        parse_flow_control("xon_xoff");
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    CHECK(threw, "should throw for invalid flow control");
}

int main()
{
    std::cout << "=== SerialUtility Tests ===" << std::endl;

    test_hex_to_bytes();
    test_bytes_to_hex();
    test_bytes_to_utf8_safe();
    test_port_info_to_json();
    test_parity_name();
    test_stop_bits_name();
    test_flow_control_name();
    test_parse_parity();
    test_parse_stop_bits();
    test_parse_flow_control();

    std::cout << std::endl;
    if (failures > 0)
    {
        std::cout << failures << " test(s) FAILED." << std::endl;
        return 1;
    }
    std::cout << "All tests PASSED." << std::endl;
    return 0;
}
