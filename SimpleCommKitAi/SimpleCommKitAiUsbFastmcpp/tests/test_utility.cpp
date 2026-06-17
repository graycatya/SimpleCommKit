/// Unit tests for USB utility functions (hex_to_bytes, bytes_to_hex, JSON converters, etc.).
/// No hardware required.

#include "usb_state.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace SimpleCommKitAiUsbFastmcpp;

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

static void test_transfer_type_name()
{
    std::cout << "\n=== transfer_type_name ===" << std::endl;

    check("control", std::string(transfer_type_name(
        SimpleCommKit::SimpleCommKitUsbTransferType::Control)) == "control");
    check("bulk", std::string(transfer_type_name(
        SimpleCommKit::SimpleCommKitUsbTransferType::Bulk)) == "bulk");
    check("interrupt", std::string(transfer_type_name(
        SimpleCommKit::SimpleCommKitUsbTransferType::Interrupt)) == "interrupt");
    check("isochronous", std::string(transfer_type_name(
        SimpleCommKit::SimpleCommKitUsbTransferType::Isochronous)) == "isochronous");
}

static void test_parse_transfer_type()
{
    std::cout << "\n=== parse_transfer_type ===" << std::endl;

    check("bulk -> Bulk", parse_transfer_type("bulk") ==
        SimpleCommKit::SimpleCommKitUsbTransferType::Bulk);
    check("interrupt -> Interrupt", parse_transfer_type("interrupt") ==
        SimpleCommKit::SimpleCommKitUsbTransferType::Interrupt);
    check("isochronous -> Isochronous", parse_transfer_type("isochronous") ==
        SimpleCommKit::SimpleCommKitUsbTransferType::Isochronous);
    check("control -> Control", parse_transfer_type("control") ==
        SimpleCommKit::SimpleCommKitUsbTransferType::Control);

    // Unknown throws
    {
        bool threw = false;
        try { parse_transfer_type("unknown"); } catch (const std::runtime_error&) { threw = true; }
        check("unknown type throws", threw);
    }
}

static void test_device_info_to_json()
{
    std::cout << "\n=== device_info_to_json ===" << std::endl;

    SimpleCommKit::SimpleCommKitUsbDeviceInfo info;
    info.vendor_id  = 0x1234;
    info.product_id = 0x5678;
    info.manufacturer_string = "Test Corp";
    info.product_string      = "Test Device";
    info.serial_number       = "ABC123";
    info.bus_number          = 1;
    info.device_address      = 3;
    info.path                = "1:3";

    auto json = device_info_to_json(info);
    check("vendor_id", json["vendor_id"] == 0x1234);
    check("product_id", json["product_id"] == 0x5678);
    check("manufacturer_string", json["manufacturer_string"] == "Test Corp");
    check("product_string", json["product_string"] == "Test Device");
    check("serial_number", json["serial_number"] == "ABC123");
    check("bus_number", json["bus_number"] == 1);
    check("device_address", json["device_address"] == 3);
    check("path", json["path"] == "1:3");
}

static void test_endpoint_info_to_json()
{
    std::cout << "\n=== endpoint_info_to_json ===" << std::endl;

    SimpleCommKit::SimpleCommKitUsbEndpointInfo ep;
    ep.endpoint_address = 0x81;
    ep.attributes       = 0x03; // Interrupt
    ep.max_packet_size  = 64;
    ep.interval         = 10;
    ep.is_in            = true;

    auto json = endpoint_info_to_json(ep);
    check("endpoint_address", json["endpoint_address"] == 0x81);
    check("is_in", json["is_in"] == true);
    check("direction", json["direction"] == "IN");
    check("is_interrupt", json["is_interrupt"] == true);
    check("max_packet_size", json["max_packet_size"] == 64);
}

static void test_interface_info_to_json()
{
    std::cout << "\n=== interface_info_to_json ===" << std::endl;

    SimpleCommKit::SimpleCommKitUsbInterfaceInfo iface;
    iface.interface_number  = 0;
    iface.alternate_setting = 0;
    iface.num_endpoints     = 2;
    iface.interface_class   = 0x03; // HID
    iface.interface_subclass = 0x00;
    iface.interface_protocol = 0x00;
    iface.interface_string  = "HID Interface";

    SimpleCommKit::SimpleCommKitUsbEndpointInfo ep1;
    ep1.endpoint_address = 0x81;
    ep1.attributes       = 0x03;
    ep1.is_in            = true;

    SimpleCommKit::SimpleCommKitUsbEndpointInfo ep2;
    ep2.endpoint_address = 0x01;
    ep2.attributes       = 0x03;
    ep2.is_in            = false;

    iface.endpoints = {ep1, ep2};

    auto json = interface_info_to_json(iface);
    check("interface_number", json["interface_number"] == 0);
    check("interface_class", json["interface_class"] == 0x03);
    check("num_endpoints", json["num_endpoints"] == 2);
    check("endpoints_size", json["endpoints"].size() == 2);
}

static void test_iso_packet_to_json()
{
    std::cout << "\n=== iso_packet_to_json ===" << std::endl;

    SimpleCommKit::SimpleCommKitUsbIsoPacketResult pkt;
    pkt.length        = 512;
    pkt.actual_length = 480;
    pkt.status        = 0;

    auto json = iso_packet_to_json(pkt);
    check("length", json["length"] == 512);
    check("actual_length", json["actual_length"] == 480);
    check("status", json["status"] == 0);
}

int main()
{
    std::cout << "SimpleCommKitAiUsbFastmcpp Utility Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    test_hex_to_bytes();
    test_bytes_to_hex();
    test_bytes_to_utf8_safe();
    test_transfer_type_name();
    test_parse_transfer_type();
    test_device_info_to_json();
    test_endpoint_info_to_json();
    test_interface_info_to_json();
    test_iso_packet_to_json();

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
