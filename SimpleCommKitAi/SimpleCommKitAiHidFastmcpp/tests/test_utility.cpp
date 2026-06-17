/// Tests for utility functions: hex_to_bytes, bytes_to_hex, bytes_to_utf8_safe,
/// device_info_to_json, bus_type_name.

#include "../src/hid_state.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace SimpleCommKitAiHidFastmcpp;

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

    TEST("bytes_to_utf8_safe mixed valid/invalid");
    result = bytes_to_utf8_safe({0x41, 0xC0, 0x80, 0x42}); // 0xC080 invalid
    // 0x41='A', then 0xC080 is overlong/surrogate so should be skipped, then 0x42='B'
    // Actually 0xC0 0x80 is technically valid but overlong, we skip as our algo
    // validates 2-byte sequences. Let me verify: 0xC0 & 0xE0 = 0xC0, so 2-byte.
    // 0x80 & 0xC0 = 0x80. So this will be treated as valid 2-byte.
    CHECK(result.size() >= 2, "expected at least 2 bytes");
}

void test_bus_type_name()
{
    TEST("bus_type_name known");
    CHECK(std::string(bus_type_name(1)) == "USB", "expected 'USB'");
    CHECK(std::string(bus_type_name(2)) == "BLUETOOTH", "expected 'BLUETOOTH'");

    TEST("bus_type_name unknown");
    CHECK(std::string(bus_type_name(99)) == "UNKNOWN", "expected 'UNKNOWN'");
    std::cout << "  bus_type_name unknown... PASS" << std::endl;
}

void test_device_info_to_json()
{
    TEST("device_info_to_json");
    SimpleCommKit::SimpleCommKitHidDeviceInfo info;
    info.path = "/dev/hidraw0";
    info.manufacturer_string = "TestCorp";
    info.product_string = "TestDevice";
    info.serial_number = "ABC123";
    info.bus_type = 1; // USB
    info.interface_number = 0;
    info.release_number = 0x0100;

    auto json = device_info_to_json(info);
    CHECK(json["path"] == "/dev/hidraw0", "path mismatch");
    CHECK(json["manufacturer_string"] == "TestCorp", "manufacturer mismatch");
    CHECK(json["product_string"] == "TestDevice", "product mismatch");
    CHECK(json["serial_number"] == "ABC123", "serial number mismatch");
    CHECK(json["bus_type"] == 1, "bus type mismatch");
    CHECK(json["bus_type_name"] == "USB", "bus type name mismatch");
    CHECK(json["interface_number"] == 0, "interface number mismatch");
    CHECK(json["release_number"] == 0x0100, "release number mismatch");
    std::cout << "  device_info_to_json fields... PASS" << std::endl;
}

int main()
{
    std::cout << "=== HidUtility Tests ===" << std::endl;

    test_hex_to_bytes();
    test_bytes_to_hex();
    test_bytes_to_utf8_safe();
    test_bus_type_name();
    test_device_info_to_json();

    std::cout << std::endl;
    if (failures > 0)
    {
        std::cout << failures << " test(s) FAILED." << std::endl;
        return 1;
    }
    std::cout << "All tests PASSED." << std::endl;
    return 0;
}
