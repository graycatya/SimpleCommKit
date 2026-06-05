"""MCP tool tests for SimpleCommKitAiHid."""

import pytest
from unittest.mock import MagicMock, patch
from typing import Dict, List


# Reuse mocks from conftest
from conftest import MockSimpleCommKitHid, MockHidDeviceInfo


class TestMcpTools:
    """Tests for MCP tool functions using mock HID."""

    def test_get_available_devices(self):
        """get_available_devices should return a list of devices."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._devices = [
            MockHidDeviceInfo(path="/dev/hidraw0", product_string="Mouse"),
            MockHidDeviceInfo(path="/dev/hidraw1", product_string="Keyboard"),
        ]
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import get_available_devices
        results = get_available_devices()
        assert isinstance(results, list)
        assert len(results) == 2
        assert results[0]["path"] == "/dev/hidraw0"
        assert results[0]["product_string"] == "Mouse"
        assert results[1]["product_string"] == "Keyboard"

    def test_get_available_devices_filtered(self):
        """get_available_devices should accept VID/PID filter."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import get_available_devices
        results = get_available_devices(vendor_id=0x1234, product_id=0x5678)
        assert isinstance(results, list)

    def test_open_device_by_path(self):
        """open_device should open a device by path."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._devices = [MockHidDeviceInfo(path="/dev/hidraw0")]
        hid_state.hid = mock_hid
        hid_state._initialized = True
        hid_state._read_buffer = {}

        from SimpleCommKitAiHid.mcp import open_device
        result = open_device(path="/dev/hidraw0")
        assert "Opened" in result["message"]
        assert result["path"] == "/dev/hidraw0"

    def test_open_device_already_open(self):
        """open_device should return success for already-open device."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._open_paths = ["/dev/hidraw0"]
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import open_device
        result = open_device(path="/dev/hidraw0")
        assert "already open" in result["message"]

    def test_open_device_by_vid_pid(self):
        """open_device should open by VID/PID."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._devices = [MockHidDeviceInfo(path="/dev/hidraw0")]
        hid_state.hid = mock_hid
        hid_state._initialized = True
        hid_state._read_buffer = {}

        from SimpleCommKitAiHid.mcp import open_device
        result = open_device(vendor_id=0x1234, product_id=0x5678, readable=False)
        assert "Opened" in result["message"]

    def test_open_device_no_args(self):
        """open_device should raise when no path or VID/PID provided."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import open_device
        with pytest.raises(RuntimeError, match="Must provide"):
            open_device()

    def test_close_device(self):
        """close_device should close a device."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._open_paths = ["/dev/hidraw0"]
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import close_device
        result = close_device(path="/dev/hidraw0")
        assert "Closed" in result["message"]

    def test_close_all_devices(self):
        """close_device should close all when no path given."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._open_paths = ["/dev/hidraw0", "/dev/hidraw1"]
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import close_device
        result = close_device()
        assert "Closed all devices" in result["message"]

    def test_write(self):
        """write should write hex data to a device."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._open_paths = ["/dev/hidraw0"]
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import write
        result = write("/dev/hidraw0", "00010203")
        assert result["bytes_written"] == 4

    def test_write_device_not_open(self):
        """write should raise when device not open."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import write
        with pytest.raises(RuntimeError, match="not open"):
            write("/dev/hidraw0", "00")

    def test_write_invalid_hex(self):
        """write should raise for invalid hex."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._open_paths = ["/dev/hidraw0"]
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import write
        with pytest.raises(RuntimeError, match="Invalid hex"):
            write("/dev/hidraw0", "zzz")

    def test_send_feature_report(self):
        """send_feature_report should send hex data to a device."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._open_paths = ["/dev/hidraw0"]
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import send_feature_report
        result = send_feature_report("/dev/hidraw0", "0501")
        assert "Feature report sent" in result["message"]

    def test_start_hotplug(self):
        """start_hotplug should start hotplug detection."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import start_hotplug
        result = start_hotplug(vendor_id=0, product_id=0, poll_interval_ms=500)
        assert "started" in result["message"].lower()
        assert result["poll_interval_ms"] == 500

    def test_stop_hotplug(self):
        """stop_hotplug should stop hotplug detection."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._hotplug_active = True
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import stop_hotplug
        result = stop_hotplug()
        assert "stopped" in result["message"].lower()

    def test_get_device_list(self):
        """get_device_list should return cached devices."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._devices = [
            MockHidDeviceInfo(path="/dev/hidraw0", product_string="Test Device")
        ]
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import get_device_list
        results = get_device_list()
        assert len(results) == 1
        assert results[0]["path"] == "/dev/hidraw0"

    def test_get_open_paths(self):
        """get_open_paths should return open device paths."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        mock_hid._open_paths = ["/dev/hidraw0", "/dev/hidraw1"]
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import get_open_paths
        result = get_open_paths()
        assert result["open_count"] == 2
        assert len(result["open_paths"]) == 2

    def test_get_read_data(self):
        """get_read_data should return and clear buffered data."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        hid_state.hid = mock_hid
        hid_state._initialized = True
        hid_state._read_buffer = {
            "/dev/hidraw0": [
                {"data_hex": "ab", "data_utf8": "", "data_length": 1},
                {"data_hex": "cd", "data_utf8": "", "data_length": 1},
            ]
        }

        from SimpleCommKitAiHid.mcp import get_read_data
        result = get_read_data("/dev/hidraw0")
        assert len(result) == 2
        assert result[0]["data_hex"] == "ab"

        # Buffer should be cleared
        result2 = get_read_data("/dev/hidraw0")
        assert result2 == []

    def test_get_read_data_empty(self):
        """get_read_data should return empty for unknown path."""
        from SimpleCommKitAiHid.mcp import hid_state
        hid_state._read_buffer = {}

        from SimpleCommKitAiHid.mcp import get_read_data
        result = get_read_data("unknown")
        assert result == []

    def test_set_read_config(self):
        """set_read_config should update read parameters."""
        from SimpleCommKitAiHid.mcp import hid_state

        mock_hid = MockSimpleCommKitHid()
        hid_state.hid = mock_hid
        hid_state._initialized = True

        from SimpleCommKitAiHid.mcp import set_read_config
        result = set_read_config("/dev/hidraw0", poll_interval_ms=200, data_length=128)
        assert result["read_poll_interval_ms"] >= 0
        assert result["read_data_length"] >= 0
