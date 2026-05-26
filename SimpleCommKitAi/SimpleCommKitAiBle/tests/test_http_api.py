"""HTTP API endpoint tests for SimpleCommKitAiBle."""

import pytest
from unittest.mock import patch, MagicMock
from fastapi.testclient import TestClient


# Patch BLE dependencies BEFORE importing http module
@pytest.fixture(autouse=True)
def patch_ble_for_http():
    """Patch SimpleCommKitAiBle imports before http module is loaded."""
    mock_central = MagicMock()
    mock_central.bluetooth_enabled.return_value = True
    mock_central.return_value = mock_central
    mock_central.get_adapters.return_value = [MagicMock(identifier="adapter-0", address="00:11:22:33:44:55")]
    mock_central.get_current_adapter.return_value = MagicMock()
    mock_central.adapter_scan_for = MagicMock()
    mock_central.adapter_get_scan_results.return_value = [
        MagicMock(
            identifier="Test Device",
            address="AA:BB:CC:DD:EE:FF",
            rssi=-45,
            address_type=0,
            manufacturer_data={0x004C: b"\x02\x15"}
        )
    ]
    mock_central.peripheral_connect = MagicMock()
    mock_central.peripheral_disconnect = MagicMock()
    mock_central.peripheral_is_connected.return_value = True
    mock_central.peripheral_services.return_value = [
        MagicMock(uuid="00001800-0000-1000-8000-00805f9b34fb",
                   characteristics=[MagicMock(uuid="00002a00-0000-1000-8000-00805f9b34fb")])
    ]
    mock_central.peripheral_get_mtu.return_value = 247
    mock_central.peripheral_read.return_value = b"hello"
    mock_central.peripheral_write_request = MagicMock()
    mock_central.peripheral_write_command = MagicMock()
    mock_central.peripheral_notify = MagicMock(side_effect=lambda s, c, cb: cb(b"notify_data"))
    mock_central.peripheral_indicate = MagicMock(side_effect=lambda s, c, cb: cb(b"indicate_data"))
    mock_central.peripheral_unsubscribe = MagicMock()
    mock_central.set_callback_error = MagicMock()

    adapter_cls = MagicMock()
    adapter_cls.return_value = MagicMock(identifier="adapter-0", address="00:11:22:33:44:55")

    peripheral_cls = MagicMock()
    peripheral_cls.return_value = MagicMock(identifier="Test Device", address="AA:BB:CC:DD:EE:FF")

    with patch.dict("sys.modules", {}):
        with patch("SimpleCommKitAiBle.BleCentral", mock_central), \
             patch("SimpleCommKitAiBle.Adapter", adapter_cls.__class__), \
             patch("SimpleCommKitAiBle.Peripheral", peripheral_cls.__class__), \
             patch("SimpleCommKitAiBle.get_error_description", lambda c: f"Error {c}"):
            import SimpleCommKitAiBle
            SimpleCommKitAiBle.BleCentral = mock_central
            SimpleCommKitAiBle.Adapter = MagicMock
            SimpleCommKitAiBle.Peripheral = MagicMock
            SimpleCommKitAiBle.get_error_description = lambda c: f"Error {c}"
            yield


@pytest.fixture
def http_client():
    """Create a TestClient for the FastAPI app."""
    # Force reload http module with our patches
    import importlib
    import SimpleCommKitAiBle.http as http_mod
    importlib.reload(http_mod)
    return TestClient(http_mod.app)


class TestHttpApi:
    """Tests for HTTP REST API endpoints."""

    def test_root_health_check(self, http_client):
        """GET / should return health status."""
        response = http_client.get("/")
        assert response.status_code == 200
        data = response.json()
        assert "message" in data
        assert "running" in data["message"].lower()

    def test_get_adapters(self, http_client):
        """GET /adapters should return adapter list."""
        response = http_client.get("/adapters")
        assert response.status_code == 200
        data = response.json()
        assert isinstance(data, list)
        assert len(data) >= 0
        if data:
            assert "identifier" in data[0]
            assert "address" in data[0]

    def test_scan(self, http_client):
        """POST /scan should return scan results."""
        response = http_client.post("/scan?timeout_ms=1000")
        assert response.status_code == 200
        data = response.json()
        assert isinstance(data, list)
        if data:
            assert "address" in data[0]
            assert "rssi" in data[0]
            assert "manufacturer_data" in data[0]

    def test_connect_device(self, http_client):
        """POST /connect/{address} should connect to a scanned device."""
        # First scan
        http_client.post("/scan?timeout_ms=1000")
        # Then connect
        response = http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        assert response.status_code == 200
        data = response.json()
        assert "message" in data
        assert "Connected" in data["message"]

    def test_connect_already_connected(self, http_client):
        """POST /connect/{address} twice should return already connected."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        response = http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        assert response.status_code == 200
        assert "Already connected" in response.json()["message"]

    def test_connect_device_not_found(self, http_client):
        """POST /connect/{address} without scan should return 404."""
        response = http_client.post("/connect/UNKNOWN:ADDRESS")
        assert response.status_code == 404

    def test_get_device_info(self, http_client):
        """GET /device/{address} should return device services."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        response = http_client.get("/device/AA:BB:CC:DD:EE:FF")
        assert response.status_code == 200
        data = response.json()
        assert data["connected"] is True
        assert "services" in data
        assert "mtu" in data

    def test_read_characteristic(self, http_client):
        """POST /device/{address}/read/{svc}/{ch} should read data."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        response = http_client.post(f"/device/AA:BB:CC:DD:EE:FF/read/{svc}/{ch}")
        assert response.status_code == 200
        data = response.json()
        assert "data_hex" in data

    def test_write_characteristic(self, http_client):
        """POST /device/{address}/write/{svc}/{ch} should write data."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        response = http_client.post(
            f"/device/AA:BB:CC:DD:EE:FF/write/{svc}/{ch}",
            json={"data": "48656c6c6f"}  # "Hello" in hex
        )
        assert response.status_code == 200
        assert "Write successful" in response.json()["message"]

    def test_write_invalid_hex(self, http_client):
        """POST /device/{address}/write with invalid hex should return 400."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        response = http_client.post(
            f"/device/AA:BB:CC:DD:EE:FF/write/{svc}/{ch}",
            json={"data": "not-a-hex-string!"}
        )
        assert response.status_code == 400

    def test_write_command(self, http_client):
        """POST /device/{address}/write_command/{svc}/{ch} should work."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        response = http_client.post(
            f"/device/AA:BB:CC:DD:EE:FF/write_command/{svc}/{ch}",
            json={"data": "01ff"}
        )
        assert response.status_code == 200
        assert "Write command successful" in response.json()["message"]

    def test_notify_subscribe(self, http_client):
        """POST /device/{address}/notify/{svc}/{ch} should subscribe."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        response = http_client.post(f"/device/AA:BB:CC:DD:EE:FF/notify/{svc}/{ch}")
        assert response.status_code == 200
        assert "Subscribed" in response.json()["message"]

    def test_get_notifications(self, http_client):
        """GET /device/{address}/notifications should return buffered data."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        http_client.post(f"/device/AA:BB:CC:DD:EE:FF/notify/{svc}/{ch}")
        response = http_client.get("/device/AA:BB:CC:DD:EE:FF/notifications")
        assert response.status_code == 200
        data = response.json()
        assert isinstance(data, list)
        if data:
            assert "data_hex" in data[0]
            assert data[0]["type"] == "notification"

    def test_unsubscribe(self, http_client):
        """POST /device/{address}/unsubscribe/{svc}/{ch} should unsubscribe."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        svc = "00001800-0000-1000-8000-00805f9b34fb"
        ch = "00002a00-0000-1000-8000-00805f9b34fb"
        http_client.post(f"/device/AA:BB:CC:DD:EE:FF/notify/{svc}/{ch}")
        response = http_client.post(f"/device/AA:BB:CC:DD:EE:FF/unsubscribe/{svc}/{ch}")
        assert response.status_code == 200
        assert "Unsubscribed" in response.json()["message"]

    def test_disconnect(self, http_client):
        """POST /disconnect/{address} should disconnect."""
        http_client.post("/scan?timeout_ms=1000")
        http_client.post("/connect/AA:BB:CC:DD:EE:FF")
        response = http_client.post("/disconnect/AA:BB:CC:DD:EE:FF")
        assert response.status_code == 200
        assert "Disconnected" in response.json()["message"]

    def test_disconnect_not_found(self, http_client):
        """POST /disconnect/{address} for unknown device should return 404."""
        response = http_client.post("/disconnect/UNKNOWN:ADDRESS")
        assert response.status_code == 404
