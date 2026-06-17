/// Placeholder test for HidState (requires actual HID hardware).
/// Tests that the singleton pattern works and init() handles the no-hardware case.

#include "../src/hid_state.hpp"

#include <cassert>
#include <iostream>

using namespace SimpleCommKitAiHidFastmcpp;

static int failures = 0;

#define CHECK(cond, msg)    \
    if (!(cond))            \
    {                       \
        std::cout << "FAIL: " << msg << std::endl; \
        ++failures;         \
    }                       \
    else                    \
      std::cout << "PASS" << std::endl

int main()
{
    std::cout << "=== HidState Tests ===" << std::endl;

    // Test singleton
    auto& s1 = HidState::instance();
    auto& s2 = HidState::instance();
    std::cout << "  singleton identity... ";
    CHECK(&s1 == &s2, "singleton identity failed");

    // Test init without hardware (may return false, but should not crash)
    std::cout << "  init no crash... ";
    bool ok = s1.init();
    CHECK(true, "init did not crash (result: " + std::string(ok ? "ready" : "no hardware") + ")");

    // Test ensure_hid behavior when no hardware
    std::cout << "  ensure_hid... ";
    try
    {
        s1.ensure_hid();
        CHECK(true, "ensure_hid succeeded");
    }
    catch (const std::runtime_error& e)
    {
        CHECK(true, "ensure_hid threw (expected without hardware): " + std::string(e.what()));
    }

    // Test read buffer operations
    std::cout << "  drain_read_buffer empty... ";
    auto entries = s1.drain_read_buffer("/nonexistent");
    CHECK(entries.empty(), "expected empty buffer");

    std::cout << "  remove_read_buffer no-op... ";
    s1.remove_read_buffer("/nonexistent");
    std::cout << "PASS" << std::endl;

    std::cout << "  clear_read_buffers no-op... ";
    s1.clear_read_buffers();
    std::cout << "PASS" << std::endl;

    std::cout << std::endl;
    if (failures > 0)
    {
        std::cout << failures << " test(s) FAILED." << std::endl;
        return 1;
    }
    std::cout << "All tests PASSED." << std::endl;
    return 0;
}
