/// Unit tests for WebSocket utility functions (hex_to_bytes, bytes_to_hex, etc.).
/// No hardware or network required.

#include "ws_state.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace SimpleCommKitAiWebSocketClientFastmcpp;

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

int main()
{
    std::cout << "SimpleCommKitAiWebSocketClientFastmcpp Utility Tests" << std::endl;
    std::cout << "==================================================" << std::endl;

    test_hex_to_bytes();
    test_bytes_to_hex();
    test_bytes_to_utf8_safe();

    std::cout << "\n==================================================" << std::endl;
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
