"""MCP tool tests for SimpleCommKitAiBle."""

import pytest
from unittest.mock import MagicMock, patch
from typing import Dict, List


# Mock BLE classes
class MockAdapter:
    def __init__(self, identifier="adapter-0", address="00:11:22:33:44:55"):
        self.identifier = identifier
        self.address = address


class MockPeripheral:
    def __init__(self, identifier="Test Device", address="AA:BB:CC:DD:EE:FF", rssi=-50):
        self.identifier = identifier
        self.address = address
        self.rssi = rssi
        self.address_type = 0
        self.manufacturer_data = {0x004C: b"\x02\x15test"}


class MockCharacteristic:
    def __init__(self, uuid="00002a00-0000-1000-8000-00805f9b34fb",
                 can_read=True, can_write_request=True,
                 can_write_command=False, can_notify=True, can_indicate=False):
        self.uuid = uuid
        self.can_read = can_read
        self.can_write_request = can_write_request
        self.can_write_command = can_write_command
        self.can_notify = can_notify
        self.can_indicate = can_indicate
        self.descriptors = []


class MockService:
    def __init__(self, uuid="00001800-0000-1000-8000-00805f9b34fb"):
        self.uuid = uuid
        self.characteristics = [MockCharacteristic()]


class MockBleCentral:
    def __init__(self):
        self._adapters = [MockAdapter()]
        self._current_adapter = self._adapters[0]
        self._scan_results = [MockPeripheral()]
        self._current_peripheral = None
        self._connected = False
        self._error_callback = None

    @staticmethod
    def bluetooth_enabled():
        return True

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
        return b"hello"

    def peripheral_write_request(self, service_uuid, char_uuid, data):
        pass

    def peripheral_write_command(self, service_uuid, char_uuid, data):
        pass

    def peripheral_notify(self, service_uuid, char_uuid, callback):
        callback(b"notify_data_1")
        callback(b"notify_data_2")

    def peripheral_indicate(self, service_uuid, char_uuid, callback):
        callback(b"indicate_data")

    def peripheral_unsubscribe(self, service_uuid, char_uuid):
        pass

    def peripheral_read_descriptor(self, service_uuid, char_uuid, descriptor_uuid):
        return b"desc_data"

    def peripheral_write_descriptor(self, service_uuid, char_uuid, descriptor_uuid, data):
        pass

    def set_callback_error(self, callback):
        self._error_callback = callback


def get_error_description(code):
    return f"Error {code}"


@pytest.fixture
def ble_state_ctx():
    """Patch the ble_state and mcp in the mcp module."""
    with patch("SimpleCommKitAiBle.mcp.BleState") as mock_ble_state_cls, \
         patch("SimpleCommKitAiBle.mcp.mcp") as mock_mcp_instance, \
         patch("SimpleCommKitAiBle.mcp.ble_state") as mock_module_ble_state:
        # Setup mock BleState instance
        mock_state = MagicMock()
        mock_state.central = MockBleCentral()
        mock_state.adapters = [MockAdapter()]
        mock_state.current_adapter = MockAdapter()
        mock_state.scan_results = [MockPeripheral()]
        mock_state.connected = {}
        mock_state.notifications = {}
        mock_state.refresh_adapters = MagicMock()
        mock_ble_state_cls.return_value = mock_state
        mock_module_ble_state.__class__ = MagicMock
        yield mock_state


class TestMcpTools:
    """Tests for MCP tool functions using mock BLE."""

    def test_bluetooth_enabled(self):
        """bluetooth_enabled should return enabled=True."""
        from SimpleCommKitAiBle.mcp import bluetooth_enabled
        result = bluetooth_enabled()
        assert result == {"enabled": True}

    def test_scan_for(self):
        """scan_for should return a list of peripherals."""
        # Directly test with a mock central
        from SimpleCommKitAiBle.mcp import ble_state

        # Create a mock BleState with mock central
        mock_central = MockBleCentral()
        ble_state.central = mock_central
        ble_state.adapters = mock_central.get_adapters()
        ble_state.current_adapter = mock_central.get_current_adapter()
        ble_state.scan_results = []
        ble_state.refresh_adapters = lambda: None

        from SimpleCommKitAiBle.mcp import scan_for
        results = scan_for(timeout_ms=1000)
        assert isinstance(results, list)
        assert len(results) > 0
        assert "address" in results[0]
        assert "rssi" in results[0]
        assert "manufacturer_data" in results[0]

    def test_scan_for_no_adapter(self):
        """scan_for should raise when no adapter is available."""
        from SimpleCommKitAiBle.mcp import ble_state
        ble_state.current_adapter = None
        ble_state.refresh_adapters = lambda: None

        from SimpleCommKitAiBle.mcp import scan_for
        with pytest.raises(RuntimeError, match="No Bluetooth adapter"):
            scan_for()

    def test_connect(self):
        """connect should connect to a scanned device."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._scan_results = [MockPeripheral()]
        ble_state.central = mock_central
        ble_state.scan_results = mock_central._scan_results
        ble_state.connected = {}
        ble_state.notifications = {}

        from SimpleCommKitAiBle.mcp import connect
        result = connect("AA:BB:CC:DD:EE:FF")
        assert "Connected" in result["message"]
        assert result["address"] == "AA:BB:CC:DD:EE:FF"

    def test_connect_already_connected(self):
        """connect should return already connected message."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._scan_results = [MockPeripheral()]
        ble_state.central = mock_central
        ble_state.scan_results = mock_central._scan_results
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}
        ble_state.notifications = {}

        from SimpleCommKitAiBle.mcp import connect
        result = connect("AA:BB:CC:DD:EE:FF")
        assert "Already connected" in result["message"]

    def test_connect_not_found(self):
        """connect should raise when device not in scan results."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._scan_results = []
        ble_state.central = mock_central
        ble_state.scan_results = []
        ble_state.connected = {}
        ble_state.notifications = {}

        from SimpleCommKitAiBle.mcp import connect
        with pytest.raises(RuntimeError, match="not found"):
            connect("AA:BB:CC:DD:EE:FF")

    def test_disconnect(self):
        """disconnect should disconnect from a connected device."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = True
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}
        ble_state.notifications = {"AA:BB:CC:DD:EE:FF": []}

        from SimpleCommKitAiBle.mcp import disconnect
        result = disconnect("AA:BB:CC:DD:EE:FF")
        assert "Disconnected" in result["message"]

    def test_disconnect_not_found(self):
        """disconnect should raise when device not connected."""
        from SimpleCommKitAiBle.mcp import ble_state
        ble_state.connected = {}
        ble_state.notifications = {}

        from SimpleCommKitAiBle.mcp import disconnect
        with pytest.raises(RuntimeError, match="not found"):
            disconnect("AA:BB:CC:DD:EE:FF")

    def test_services(self):
        """services should return GATT services."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = True
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}

        from SimpleCommKitAiBle.mcp import services
        result = services("AA:BB:CC:DD:EE:FF")
        assert result["connected"] is True
        assert result["address"] == "AA:BB:CC:DD:EE:FF"
        assert "services" in result
        assert "mtu" in result

    def test_services_not_connected_device(self):
        """services should return connected=False for disconnected device."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = False
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}

        from SimpleCommKitAiBle.mcp import services
        result = services("AA:BB:CC:DD:EE:FF")
        assert result["connected"] is False

    def test_services_not_found(self):
        """services should raise when device not found."""
        from SimpleCommKitAiBle.mcp import ble_state
        ble_state.central = MockBleCentral()
        ble_state.connected = {}

        from SimpleCommKitAiBle.mcp import services
        with pytest.raises(RuntimeError, match="not found"):
            services("AA:BB:CC:DD:EE:FF")

    def test_read(self):
        """read should return characteristic data."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = True
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}

        from SimpleCommKitAiBle.mcp import read
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        result = read("AA:BB:CC:DD:EE:FF", svc, ch)
        assert "data_hex" in result
        assert "data_utf8" in result

    def test_read_not_found(self):
        """read should raise when device not found."""
        from SimpleCommKitAiBle.mcp import ble_state
        ble_state.central = MockBleCentral()
        ble_state.connected = {}

        from SimpleCommKitAiBle.mcp import read
        with pytest.raises(RuntimeError, match="not found"):
            read("AA:BB:CC:DD:EE:FF", "svc", "ch")

    def test_write_request(self):
        """write_request should write hex data with response."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = True
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}

        from SimpleCommKitAiBle.mcp import write_request
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        result = write_request("AA:BB:CC:DD:EE:FF", svc, ch, "48656c6c6f")
        assert "Write successful" in result["message"]

    def test_write_request_invalid_hex(self):
        """write_request should raise for invalid hex."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = True
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}

        from SimpleCommKitAiBle.mcp import write_request
        with pytest.raises(RuntimeError, match="Invalid hex"):
            write_request("AA:BB:CC:DD:EE:FF", "svc", "ch", "zzz")

    def test_write_command(self):
        """write_command should write hex data without response."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = True
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}

        from SimpleCommKitAiBle.mcp import write_command
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        result = write_command("AA:BB:CC:DD:EE:FF", svc, ch, "01ff")
        assert "Write command successful" in result["message"]

    def test_notify(self):
        """notify should subscribe to notifications."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = True
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}
        ble_state.notifications = {"AA:BB:CC:DD:EE:FF": []}

        from SimpleCommKitAiBle.mcp import notify
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        result = notify("AA:BB:CC:DD:EE:FF", svc, ch)
        assert "Subscribed" in result["message"]

    def test_indicate(self):
        """indicate should subscribe to indications."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = True
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}
        ble_state.notifications = {"AA:BB:CC:DD:EE:FF": []}

        from SimpleCommKitAiBle.mcp import indicate
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        result = indicate("AA:BB:CC:DD:EE:FF", svc, ch)
        assert "Subscribed" in result["message"]

    def test_get_notifications(self):
        """get_notifications should return and clear buffered data."""
        from SimpleCommKitAiBle.mcp import ble_state

        ble_state.central = MockBleCentral()
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}
        ble_state.notifications = {
            "AA:BB:CC:DD:EE:FF": [
                {"service": "svc", "characteristic": "ch",
                 "data_hex": "ab", "data_utf8": "", "type": "notification"}
            ]
        }

        from SimpleCommKitAiBle.mcp import get_notifications
        result = get_notifications("AA:BB:CC:DD:EE:FF")
        assert len(result) == 1
        assert result[0]["data_hex"] == "ab"

        # Buffer should be cleared
        result2 = get_notifications("AA:BB:CC:DD:EE:FF")
        assert result2 == []

    def test_get_notifications_empty(self):
        """get_notifications should return empty for unknown device."""
        from SimpleCommKitAiBle.mcp import ble_state
        ble_state.notifications = {}

        from SimpleCommKitAiBle.mcp import get_notifications
        result = get_notifications("UNKNOWN")
        assert result == []

    def test_unsubscribe(self):
        """unsubscribe should unsubscribe from notifications."""
        from SimpleCommKitAiBle.mcp import ble_state

        mock_central = MockBleCentral()
        mock_central._connected = True
        ble_state.central = mock_central
        ble_state.connected = {"AA:BB:CC:DD:EE:FF": True}

        from SimpleCommKitAiBle.mcp import unsubscribe
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        result = unsubscribe("AA:BB:CC:DD:EE:FF", svc, ch)
        assert "Unsubscribed" in result["message"]

    def test_unsubscribe_not_found(self):
        """unsubscribe should raise when device not found."""
        from SimpleCommKitAiBle.mcp import ble_state
        ble_state.central = MockBleCentral()
        ble_state.connected = {}

        from SimpleCommKitAiBle.mcp import unsubscribe
        with pytest.raises(RuntimeError, match="not found"):
            unsubscribe("AA:BB:CC:DD:EE:FF", "svc", "ch")
