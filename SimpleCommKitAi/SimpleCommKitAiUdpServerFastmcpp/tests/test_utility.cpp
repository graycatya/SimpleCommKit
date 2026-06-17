/// Unit tests for UDP server utility functions (no hardware / network required).
/// Tests hex_to_bytes(), bytes_to_hex(), bytes_to_utf8_safe().

#include "udp_server_state.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace SimpleCommKitAiUdpServerFastmcpp;

static int g_failures = 0;

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS()      std::cout << "PASS" << std::endl
#define FAIL(msg)   do { std::cout << "FAIL - " << msg << std::endl; ++g_failures; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); } } while(0)

// ---------------------------------------------------------------------------
// hex_to_bytes
// ---------------------------------------------------------------------------
static void test_hex_to_bytes_basic()
{
    TEST("hex_to_bytes basic");
    auto bytes = hex_to_bytes("0A1B2C");
    CHECK(bytes.size() == 3, "size mismatch");
    CHECK(bytes[0] == 0x0A, "byte 0");
    CHECK(bytes[1] == 0x1B, "byte 1");
    CHECK(bytes[2] == 0x2C, "byte 2");
    PASS();
}

static void test_hex_to_bytes_with_spaces()
{
    TEST("hex_to_bytes with spaces");
    auto bytes = hex_to_bytes("0A 1B 2C");
    CHECK(bytes.size() == 3, "size mismatch");
    CHECK(bytes[0] == 0x0A, "byte 0");
    CHECK(bytes[1] == 0x1B, "byte 1");
    CHECK(bytes[2] == 0x2C, "byte 2");
    PASS();
}

static void test_hex_to_bytes_lowercase()
{
    TEST("hex_to_bytes lowercase");
    auto bytes = hex_to_bytes("deadbeef");
    CHECK(bytes.size() == 4, "size mismatch");
    CHECK(bytes[0] == 0xDE, "byte 0");
    CHECK(bytes[1] == 0xAD, "byte 1");
    CHECK(bytes[2] == 0xBE, "byte 2");
    CHECK(bytes[3] == 0xEF, "byte 3");
    PASS();
}

static void test_hex_to_bytes_empty()
{
    TEST("hex_to_bytes empty");
    auto bytes = hex_to_bytes("");
    CHECK(bytes.empty(), "should be empty");
    PASS();
}

static void test_hex_to_bytes_odd_length()
{
    TEST("hex_to_bytes odd length");
    try
    {
        hex_to_bytes("ABC");
        FAIL("should have thrown");
    }
    catch (const std::runtime_error&)
    {
        PASS();
    }
}

static void test_hex_to_bytes_invalid_char()
{
    TEST("hex_to_bytes invalid char");
    try
    {
        hex_to_bytes("ZZ");
        FAIL("should have thrown");
    }
    catch (const std::runtime_error&)
    {
        PASS();
    }
}

// ---------------------------------------------------------------------------
// bytes_to_hex
// ---------------------------------------------------------------------------
static void test_bytes_to_hex()
{
    TEST("bytes_to_hex");
    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
    CHECK(bytes_to_hex(data) == "deadbeef", "hex mismatch");
    PASS();
}

static void test_bytes_to_hex_empty()
{
    TEST("bytes_to_hex empty");
    std::vector<uint8_t> data;
    CHECK(bytes_to_hex(data) == "", "should be empty");
    PASS();
}

// ---------------------------------------------------------------------------
// bytes_to_utf8_safe
// ---------------------------------------------------------------------------
static void test_bytes_to_utf8_safe_ascii()
{
    TEST("bytes_to_utf8_safe ASCII");
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
    CHECK(bytes_to_utf8_safe(data) == "Hello", "ASCII mismatch");
    PASS();
}

static void test_bytes_to_utf8_safe_2byte()
{
    TEST("bytes_to_utf8_safe 2-byte UTF-8");
    std::vector<uint8_t> data = {0xC3, 0xA9};
    CHECK(bytes_to_utf8_safe(data) == "\xC3\xA9", "2-byte UTF-8 mismatch");
    PASS();
}

static void test_bytes_to_utf8_safe_3byte()
{
    TEST("bytes_to_utf8_safe 3-byte UTF-8");
    std::vector<uint8_t> data = {0xE2, 0x82, 0xAC};
    CHECK(bytes_to_utf8_safe(data) == "\xE2\x82\xAC", "3-byte UTF-8 mismatch");
    PASS();
}

static void test_bytes_to_utf8_safe_invalid()
{
    TEST("bytes_to_utf8_safe invalid byte");
    std::vector<uint8_t> data = {'A', 0xFF, 'B'};
    CHECK(bytes_to_utf8_safe(data) == "AB", "invalid byte not skipped");
    PASS();
}

static void test_bytes_to_utf8_safe_empty()
{
    TEST("bytes_to_utf8_safe empty");
    std::vector<uint8_t> data;
    CHECK(bytes_to_utf8_safe(data) == "", "should be empty");
    PASS();
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main()
{
    std::cout << "=== UDP Server Utility Tests ===\n" << std::endl;

    test_hex_to_bytes_basic();
    test_hex_to_bytes_with_spaces();
    test_hex_to_bytes_lowercase();
    test_hex_to_bytes_empty();
    test_hex_to_bytes_odd_length();
    test_hex_to_bytes_invalid_char();

    test_bytes_to_hex();
    test_bytes_to_hex_empty();

    test_bytes_to_utf8_safe_ascii();
    test_bytes_to_utf8_safe_2byte();
    test_bytes_to_utf8_safe_3byte();
    test_bytes_to_utf8_safe_invalid();
    test_bytes_to_utf8_safe_empty();

    std::cout << "\n=== Results: " << g_failures << " failure(s) ===" << std::endl;
    return (g_failures > 0) ? 1 : 0;
}
