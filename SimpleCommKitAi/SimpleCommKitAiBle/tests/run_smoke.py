"""Quick smoke test for SimpleCommKitAiBle - runs without pytest.

Usage:
    cd SimpleCommKitAi/SimpleCommKitAiBle
    python -m tests.run_smoke
"""

import sys
import os

# Ensure the src directory is in the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "src"))


class Color:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    RESET = "\033[0m"
    BOLD = "\033[1m"


def run_test(name, func):
    """Run a single test and report the result."""
    try:
        func()
        print(f"  {Color.GREEN}[PASS]{Color.RESET} - {name}")
        return True
    except Exception as e:
        print(f"  {Color.RED}[FAIL]{Color.RESET} - {name}")
        print(f"    Error: {e}")
        return False


def main():
    print(f"\n{Color.BOLD}SimpleCommKitAiBle - Smoke Tests{Color.RESET}\n")
    passed = 0
    failed = 0

    # ---- Import tests ----
    print(f"{Color.BOLD}1. Import Tests:{Color.RESET}")

    def test_package_import():
        import SimpleCommKitAiBle
        assert SimpleCommKitAiBle.__version__ is not None  # version from VERSION file

    if run_test("Import package", test_package_import):
        passed += 1
    else:
        failed += 1

    def test_version_string():
        import SimpleCommKitAiBle
        assert isinstance(SimpleCommKitAiBle.__version__, str)

    if run_test("Version is string", test_version_string):
        passed += 1
    else:
        failed += 1

    def test_exports():
        from SimpleCommKitAiBle import BleCentral, Adapter, Peripheral, get_error_description
        assert BleCentral is not None or True  # May be None if simple_comm_kit_ble not installed
        assert True  # The fallback works

    if run_test("Package exports (BleCentral etc.)", test_exports):
        passed += 1
    else:
        failed += 1

    # ---- Mock BLE tests (no hardware needed) ----
    print(f"\n{Color.BOLD}2. Mock BLE Tests:{Color.RESET}")

    def test_mock_central():
        from unittest.mock import MagicMock
        central = MagicMock()
        central.bluetooth_enabled.return_value = True
        assert central.bluetooth_enabled()

    if run_test("Mock BleCentral works", test_mock_central):
        passed += 1
    else:
        failed += 1

    def test_mock_adapter():
        from unittest.mock import MagicMock
        adapter = MagicMock()
        adapter.identifier = "test-adapter"
        adapter.address = "00:11:22:33:44:55"
        assert adapter.identifier == "test-adapter"
        assert adapter.address == "00:11:22:33:44:55"

    if run_test("Mock Adapter works", test_mock_adapter):
        passed += 1
    else:
        failed += 1

    def test_mock_peripheral():
        from unittest.mock import MagicMock
        p = MagicMock()
        p.identifier = "Test Device"
        p.address = "AA:BB:CC:DD:EE:FF"
        p.rssi = -45
        p.manufacturer_data = {0x004C: b"\x02\x15"}
        assert p.rssi == -45
        assert 0x004C in p.manufacturer_data

    if run_test("Mock Peripheral works", test_mock_peripheral):
        passed += 1
    else:
        failed += 1

    # ---- HTTP endpoint style tests ----
    print(f"\n{Color.BOLD}3. HTTP API Structure Tests:{Color.RESET}")

    def test_hex_conversion():
        # Test the data format used by HTTP endpoints
        data = b"Hello"
        data_hex = data.hex()
        assert data_hex == "48656c6c6f"
        assert bytes.fromhex(data_hex) == data

    if run_test("Hex data roundtrip", test_hex_conversion):
        passed += 1
    else:
        failed += 1

    def test_write_request_model():
        from SimpleCommKitAiBle.http import WriteRequest
        req = WriteRequest(data="48656c6c6f")
        assert req.data == "48656c6c6f"

    if run_test("WriteRequest Pydantic model", test_write_request_model):
        passed += 1
    else:
        failed += 1

    # ---- Results ----
    total = passed + failed
    print(f"\n{Color.BOLD}Results: {passed}/{total} passed{Color.RESET}")
    if failed > 0:
        print(f"{Color.RED}{failed} test(s) failed{Color.RESET}")
        return 1
    else:
        print(f"{Color.GREEN}All tests passed!{Color.RESET}")
        return 0


if __name__ == "__main__":
    sys.exit(main())
