"""Pytest fixtures for SimpleCommKitAiBle tests."""

import pytest
from unittest.mock import MagicMock, patch


class MockAdapter:
    """Mock BLE adapter."""
    def __init__(self, identifier="mock-adapter-0", address="00:11:22:33:44:55"):
        self.identifier = identifier
        self.address = address


class MockCharacteristic:
    """Mock GATT characteristic."""
    def __init__(self, uuid="00002a00-0000-1000-8000-00805f9b34fb",
                 can_read=True, can_write_request=True,
                 can_write_command=False, can_notify=True, can_indicate=False):
        self.uuid = uuid
        self.can_read = can_read
        self.can_write_request = can_write_request
        self.can_write_command = can_write_command
        self.can_notify = can_notify
        self.can_indicate = can_indicate
        self.descriptors = []  # List of descriptor UUID strings


class MockService:
    """Mock GATT service."""
    def __init__(self, uuid="00001800-0000-1000-8000-00805f9b34fb", characteristics=None):
        self.uuid = uuid
        self.characteristics = characteristics or [MockCharacteristic()]


class MockPeripheral:
    """Mock BLE peripheral."""
    def __init__(self, identifier="Mock Device", address="AA:BB:CC:DD:EE:FF", rssi=-50):
        self.identifier = identifier
        self.address = address
        self.rssi = rssi
        self.address_type = 0  # public
        self.manufacturer_data = {0x004C: b"\x02\x15test"}  # Apple company ID


class MockBleCentral:
    """Mock BleCentral that simulates basic BLE operations."""

    def __init__(self):
        self._adapters = [MockAdapter()]
        self._current_adapter = None
        self._scan_results = []
        self._current_peripheral = None
        self._connected = False
        self._error_callback = None

    # ---- Static / class-level ----
    @staticmethod
    def bluetooth_enabled():
        return True

    # ---- Adapter ----
    def get_adapters(self):
        return self._adapters

    def get_current_adapter(self):
        return self._current_adapter

    def set_current_adapter(self, adapter):
        self._current_adapter = adapter

    def adapter_scan_for(self, timeout_ms):
        self._scan_results = [MockPeripheral()]

    def adapter_get_scan_results(self):
        return self._scan_results

    # ---- Peripheral ----
    def set_current_peripheral(self, peripheral):
        self._current_peripheral = peripheral

    def peripheral_connect(self):
        self._connected = True

    def peripheral_disconnect(self):
        self._connected = False

    def peripheral_is_connected(self):
        return self._connected

    def peripheral_services(self):
        return [MockService()]

    def peripheral_get_mtu(self):
        return 247

    def peripheral_read(self, service_uuid, char_uuid):
        return b"mock_data"

    def peripheral_write_request(self, service_uuid, char_uuid, data):
        pass

    def peripheral_write_command(self, service_uuid, char_uuid, data):
        pass

    def peripheral_notify(self, service_uuid, char_uuid, callback):
        # Simulate one notification
        callback(b"notify_data")

    def peripheral_indicate(self, service_uuid, char_uuid, callback):
        callback(b"indicate_data")

    def peripheral_unsubscribe(self, service_uuid, char_uuid):
        pass

    def peripheral_read_descriptor(self, service_uuid, char_uuid, descriptor_uuid):
        return b"desc_data"

    def peripheral_write_descriptor(self, service_uuid, char_uuid, descriptor_uuid, data):
        pass

    # ---- Callback ----
    def set_callback_error(self, callback):
        self._error_callback = callback


def get_error_description(code):
    """Mock error description."""
    return f"Error {code}"


@pytest.fixture
def mock_ble_module():
    """Fixture that patches SimpleCommKitAiBle with mock BLE classes."""
    with patch("SimpleCommKitAiBle.BleCentral", MockBleCentral), \
         patch("SimpleCommKitAiBle.Adapter", MockAdapter), \
         patch("SimpleCommKitAiBle.Peripheral", MockPeripheral), \
         patch("SimpleCommKitAiBle.get_error_description", get_error_description):
        yield


@pytest.fixture
def mock_ble_central():
    """Fixture that returns a fresh MockBleCentral instance."""
    return MockBleCentral()


@pytest.fixture
def mock_peripheral():
    """Fixture that returns a mock peripheral."""
    return MockPeripheral()
