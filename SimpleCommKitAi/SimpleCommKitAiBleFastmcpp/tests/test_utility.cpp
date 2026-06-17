/// Unit tests for BLE utility functions (hex_to_bytes, bytes_to_hex, etc.).
/// No hardware required.

#include "ble_state.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace SimpleCommKitAiBleFastmcpp;

static int g_failures = 0;

static void check(const char* name, bool condition)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << name << std::endl;
        ++g_failures;
    }
    else
    {
        std::cout << "PASS: " << name << std::endl;
    }
}

static void test_hex_to_bytes()
{
    std::cout << "\n=== hex_to_bytes ===" << std::endl;

    // Basic
    {
        auto result = hex_to_bytes("00FF");
        check("00FF -> [0x00, 0xFF]", result.size() == 2 && result[0] == 0x00 && result[1] == 0xFF);
    }

    // Spaces
    {
        auto result = hex_to_bytes("01 02 AA");
        check("spaced '01 02 AA'", result.size() == 3 && result[0] == 0x01 && result[1] == 0x02 && result[2] == 0xAA);
    }

    // Lowercase
    {
        auto result = hex_to_bytes("abcd");
        check("abcd -> [0xAB, 0xCD]", result.size() == 2 && result[0] == 0xAB && result[1] == 0xCD);
    }

    // Empty
    {
        auto result = hex_to_bytes("");
        check("empty string", result.empty());
    }

    // Invalid length
    {
        bool threw = false;
        try { hex_to_bytes("A"); } catch (const std::runtime_error&) { threw = true; }
        check("odd length throws", threw);
    }

    // Invalid character
    {
        bool threw = false;
        try { hex_to_bytes("ZZ"); } catch (const std::runtime_error&) { threw = true; }
        check("invalid char throws", threw);
    }
}

static void test_bytes_to_hex()
{
    std::cout << "\n=== bytes_to_hex ===" << std::endl;

    {
        std::string result = bytes_to_hex({0x00, 0xFF, 0xAB, 0x12});
        check("00FFAB12", result == "00ffab12");
    }

    {
        std::string result = bytes_to_hex({});
        check("empty vector", result.empty());
    }

    {
        std::string result = bytes_to_hex({0x01});
        check("01", result == "01");
    }
}

static void test_bytes_to_utf8_safe()
{
    std::cout << "\n=== bytes_to_utf8_safe ===" << std::endl;

    // ASCII
    {
        std::string result = bytes_to_utf8_safe({0x48, 0x65, 0x6C, 0x6C, 0x6F});
        check("Hello", result == "Hello");
    }

    // Invalid bytes are skipped
    {
        std::string result = bytes_to_utf8_safe({0xFF, 0xFE, 0x48, 0x69});
        check("skips invalid", result == "Hi");
    }

    // 2-byte UTF-8 (é = 0xC3 0xA9)
    {
        std::string result = bytes_to_utf8_safe({0x48, 0xC3, 0xA9, 0x6C, 0x6C, 0x6F});
        check("2-byte UTF-8", result.size() >= 5);
    }

    // 3-byte UTF-8 (€ = 0xE2 0x82 0xAC)
    {
        std::string result = bytes_to_utf8_safe({0xE2, 0x82, 0xAC});
        check("3-byte UTF-8 (€)", result.size() == 3);
    }

    // Empty
    {
        std::string result = bytes_to_utf8_safe({});
        check("empty", result.empty());
    }
}

static void test_address_type_name()
{
    std::cout << "\n=== address_type_name ===" << std::endl;

    check("PUBLIC", std::string(address_type_name(0)) == "PUBLIC");
    check("RANDOM", std::string(address_type_name(1)) == "RANDOM");
    check("UNSPECIFIED", std::string(address_type_name(2)) == "UNSPECIFIED");
    check("UNKNOWN", std::string(address_type_name(99)) == "UNKNOWN");
}

static void test_peripheral_to_json()
{
    std::cout << "\n=== peripheral_to_json ===" << std::endl;

    SimpleCommKit::SimpleCommKitBlePeripheral p;
    p.identifier = "test-id";
    p.address = "AA:BB:CC:DD:EE:FF";
    p.address_type = SimpleCommKit::SimpleCommKitBlePeripheralAddressType::PUBLIC;
    p.rssi = -45;
    p.manufacturer[0x004C] = {0x02, 0x15};

    auto json = peripheral_to_json(p);
    check("identifier", json["identifier"] == "test-id");
    check("address", json["address"] == "AA:BB:CC:DD:EE:FF");
    check("address_type", json["address_type"] == 0);
    check("address_type_name", json["address_type_name"] == "PUBLIC");
    check("rssi", json["rssi"] == -45);
    check("manufacturer", json["manufacturer_data"]["004c"] == "0215");
}

static void test_adapter_to_json()
{
    std::cout << "\n=== adapter_to_json ===" << std::endl;

    SimpleCommKit::SimpleCommKitBleAdapter a;
    a.dev_identifier = "hci0";
    a.dev_address = "11:22:33:44:55:66";

    auto json = adapter_to_json(a);
    check("dev_identifier", json["dev_identifier"] == "hci0");
    check("dev_address", json["dev_address"] == "11:22:33:44:55:66");
}

static void test_characteristic_to_json()
{
    std::cout << "\n=== characteristic_to_json ===" << std::endl;

    SimpleCommKit::SimpleCommKitBleCharacteristic c;
    c.uuid = "00002a19-0000-1000-8000-00805f9b34fb";
    c.descriptors_uuid = {"00002902-0000-1000-8000-00805f9b34fb"};
    c.capabilities = {"read", "notify"};
    c.can_read = true;
    c.can_write_request = false;
    c.can_write_command = false;
    c.can_notify = true;
    c.can_indicate = false;

    auto json = characteristic_to_json(c);
    check("uuid", json["uuid"] == "00002a19-0000-1000-8000-00805f9b34fb");
    check("can_read", json["can_read"] == true);
    check("can_notify", json["can_notify"] == true);
    check("can_write_request", json["can_write_request"] == false);
}

int main()
{
    std::cout << "SimpleCommKitAiBleFastmcpp Utility Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    test_hex_to_bytes();
    test_bytes_to_hex();
    test_bytes_to_utf8_safe();
    test_address_type_name();
    test_peripheral_to_json();
    test_adapter_to_json();
    test_characteristic_to_json();

    std::cout << "\n========================================" << std::endl;
    if (g_failures == 0)
    {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << g_failures << " test(s) FAILED!" << std::endl;
        return 1;
    }
}
